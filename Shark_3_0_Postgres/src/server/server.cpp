#include "0_init_system.h"
#include "postgres_db.h"
#include "server_session.h"
#include "core/system/system_function.h"
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

using json = nlohmann::json;

int main() {
  std::setlocale(LC_ALL, "");
  enableUTF8Console();

  const std::string config_dir = [] {
    if (const char *env_config_dir = std::getenv("SHARK_CONFIG_DIR"); env_config_dir && *env_config_dir)
      return std::string(env_config_dir);
    return std::string(CONFIG_DIR);
  }();

  std::ifstream file(config_dir + "/connect_db.conf");
  if (!file.is_open()) {
    std::cerr << "Missing connect_db.conf in " << config_dir << std::endl;
    std::cerr << "Copy config/connect_db.local-docker.example to config/connect_db.conf and fill real "
                 "PostgreSQL credentials."
              << std::endl;
    return 1;
  }

  json config;
  try {
    file >> config;
  } catch (const std::exception &ex) {
    std::cerr << "connect_db.conf: invalid JSON: " << ex.what() << std::endl;
    return 1;
  }

  PostgressDatabase postgress;

  postgress.setHost(config["database"]["host"]);
  postgress.setPort(config["database"]["port"]);
  postgress.setBaseName(config["database"]["dbname"]);
  postgress.setUser(config["database"]["user"]);
  postgress.setPassword(config["database"]["password"]);
  const std::string ssl_mode = config["database"].value("sslmode", "disable");

  postgress.setConnectionString("host=" + postgress.getHost() + " port=" + std::to_string(postgress.getPort()) +
                                " dbname=" + postgress.getBaseName() + " user=" + postgress.getUser() +
                                " password=" + postgress.getPassword() + " sslmode=" + ssl_mode);

  postgress.makeConnection();

  if (!postgress.isConnected()) {
    std::cerr << "[DB FATAL] Cannot connect to database." << std::endl;
    return 1;
  }

  auto conn = postgress.getConnection();

  std::cout << "Server" << std::endl;
  std::cout << "Host: " << postgress.getHost() << std::endl;
  std::cout << "Port: " << postgress.getPort() << std::endl;
  std::cout << "Base: " << postgress.getBaseName() << std::endl
            << std::endl;

  ServerSession serverSession;
  serverSession.setPgConnection(conn);

  if (!systemInitForTest(serverSession, conn)) {
    return 1;
  }
  // Start the UDP discovery server in a thread, no copying
  std::thread([&serverSession]() {
    serverSession.runUDPServerDiscovery(serverSession.getServerConnectionConfig().port);
  }).detach();

  std::cout << "[INFO] UDP discovery server started" << std::endl;

  // TCP server
  int socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_file_descriptor < 0) {
    std::cerr << "Socket creation failed!" << std::endl;
    return 1;
  }

  sockaddr_in serveraddress{};
  serveraddress.sin_family = AF_INET;
  serveraddress.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddress.sin_port = htons(serverSession.getServerConnectionConfig().port);

  int opt = 1;
  setsockopt(socket_file_descriptor, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  if (bind(socket_file_descriptor, (sockaddr *)&serveraddress, sizeof(serveraddress)) < 0) {
    perror("bind");
    std::cerr << "[Error] Failed to bind the socket" << std::endl;
    close(socket_file_descriptor);
    return 1;
  }

  if (listen(socket_file_descriptor, 5) < 0) {
    std::cerr << "[Error] Failed to listen on port" << std::endl;
    close(socket_file_descriptor);
    return 1;
  }

  std::cout << "[INFO] TCP server started on port " << serverSession.getServerConnectionConfig().port << std::endl;

  // Main loop
  while (true) {
    serverSession.runServer(socket_file_descriptor);
    if (serverSession.isConnected()) {
      serverSession.listeningClients();
    }
    usleep(50000);
  }

  close(socket_file_descriptor);
  return 0;
}

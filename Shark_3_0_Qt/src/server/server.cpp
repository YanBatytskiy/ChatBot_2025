#include "db/postgres_db.h"
#include "server_session.h"
#include "session_transport/session_transport.h"
#include "sql_commands/sql_commands.h"
#include "system/system_function.h"
#include "exceptions_cpp/network_exception.h"
#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp> 
#include <string>
#include <sys/poll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

using json = nlohmann::json;

int main() {
  std::setlocale(LC_ALL, "");
  enableUTF8Console();

  std::ifstream file(std::string(CONFIG_DIR) + "/connect_db.conf");

  json config;
  file >> config;

  PostgressDatabase postgress;

  postgress.setHost(config["database"]["host"]);
  postgress.setPort(config["database"]["port"]);
  postgress.setBaseName(config["database"]["dbname"]);
  postgress.setUser(config["database"]["user"]);
  postgress.setPassword(config["database"]["password"]);

  postgress.setConnectionString("host=" + postgress.getHost() + " port=" + std::to_string(postgress.getPort()) +
                                " dbname=" + postgress.getBaseName() + " user=" + postgress.getUser() +
                                " password=" + postgress.getPassword() + " sslmode=require");

  postgress.makeConnection();

  if (!postgress.isConnected()) {
    std::cerr << "[DB FATAL] Cannot connect to database." << std::endl;
    return 1;
  }

  auto conn = postgress.getConnection();

  std::cout << "Server" << std::endl;
  std::cout << "Host: " << postgress.getHost() << std::endl;
  std::cout << "Port: " << postgress.getPort() << std::endl;
  std::cout << "Base: " << postgress.getBaseName() << std::endl << std::endl;

  SessionTransport session_transport;
  ServerSession serverSession(SQLRequests{});
  serverSession.setPgConnection(conn);

  // 🔧 Старт UDP discovery-сервера в потоке, без копирования
  std::thread([&serverSession]() {
    serverSession.runUDPServerDiscovery(serverSession.getServerConnectionConfig().port);
  }).detach();

  std::cout << "[INFO] UDP discovery сервер запущен" << std::endl;

  // TCP-сервер
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
    std::cerr << "[Ошибка] Не удалось привязать сокет" << std::endl;
    close(socket_file_descriptor);
    return 1;
  }

  if (listen(socket_file_descriptor, 5) < 0) {
    std::cerr << "[Ошибка] Не удалось слушать порт" << std::endl;
    close(socket_file_descriptor);
    return 1;
  }

  std::cout << "[INFO] TCP-сервер запущен на порту " << serverSession.getServerConnectionConfig().port << std::endl;

  while (true) {
    try {
      session_transport.EnsureConnected(socket_file_descriptor);
    } catch (const exc::ConnectNotAcceptException &ex) {
      std::cerr << "Сервер. " << ex.what() << std::endl;
    } catch (const std::exception &ex) {
      std::cerr << "Сервер. Неизвестная ошибка. " << ex.what() << std::endl;
    }

    if (session_transport.IsConnected()) {
      int fd = session_transport.Connection();

      pollfd pfd{};
      pfd.fd = fd;
      pfd.events = POLLIN;

      int pr = ::poll(&pfd, 1, 0); // без блокировки
      if (pr > 0) {
        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
          session_transport.Reset();
        } else if (pfd.revents & POLLIN) {
          serverSession.ProcessIncoming(session_transport);
        }
      }
    }

    usleep(50000);
  }

  close(socket_file_descriptor);
  return 0;
}

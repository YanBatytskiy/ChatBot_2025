#include "0_init_system.h"
#include "chat_system/chat_system.h"
#include "server_session.h"
#include "system/system_function.h"
#include <clocale>
#include <cstdio>
#include <arpa/inet.h>
#include <iostream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

int main() {
  std::setlocale(LC_ALL, "");
  enableUTF8Console();

  ChatSystem serverSystem;
  std::cout << "Server" << std::endl;

  ServerSession serverSession(serverSystem);

  serverSystem.setIsServerStatus(true);
  serverSession.setActiveUserSrv(nullptr);
  systemInitForTest(serverSession);

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
    std::cerr << "[Error] Failed to listen on the port" << std::endl;
    close(socket_file_descriptor);
    return 1;
  }

  std::cout << "[INFO] TCP server started on port "
            << serverSession.getServerConnectionConfig().port << std::endl;

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

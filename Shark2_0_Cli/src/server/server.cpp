#include "0_init_system.h"
#include "chat_system/chat_system.h"
#include "server_session.h"
#include "system/system_function.h"
#include <arpa/inet.h>
#include <cstring>
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

  std::cout << "[INFO] TCP-сервер запущен на порту "
            << serverSession.getServerConnectionConfig().port << std::endl;

  // Главный цикл
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

#pragma once

#include "exception/network_exception.h" // подключаем твои исключения
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <cstdint>

inline void safeSend(int socketFd, const std::vector<std::uint8_t> &data) {
  ssize_t bytesSent = send(socketFd, data.data(), data.size(), 0);

  if (bytesSent < 0) {
    std::cerr << "[Ошибка] send(): " << strerror(errno) << "\n";
    close(socketFd);
    throw exc::SendDataException(); // ТВОЁ исключение
  }

  if (static_cast<std::size_t>(bytesSent) != data.size()) {
    std::cerr << "[Ошибка] send() отправил не весь буфер\n";
    close(socketFd);
    throw exc::SendDataException();
  }
}
inline std::vector<std::uint8_t> safeRecv(int socketFd,
                                          std::size_t bufferSize) {
  std::vector<std::uint8_t> buffer(bufferSize);

  ssize_t bytesReceived = recv(socketFd, buffer.data(), buffer.size(), 0);

  if (bytesReceived == 0) {
    std::cerr << "[INFO] Клиент закрыл соединение\n";
    close(socketFd);
    throw exc::ReceiveDataException(); // ТВОЁ исключение
  }

  if (bytesReceived < 0) {
    std::cerr << "[Ошибка] recv(): " << strerror(errno) << "\n";
    close(socketFd);
    throw exc::ReceiveDataException(); // ТВОЁ исключение
  }

  buffer.resize(static_cast<std::size_t>(bytesReceived));
  return buffer;
}
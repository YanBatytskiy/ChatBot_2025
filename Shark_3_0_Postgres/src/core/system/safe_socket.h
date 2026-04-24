#pragma once

#include "core/exception/network_exception.h" // include the project's exceptions
#include <cerrno>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

inline void safeSend(int socketFd, const std::vector<std::uint8_t> &data) {
  ssize_t bytesSent = send(socketFd, data.data(), data.size(), 0);

  if (bytesSent < 0) {
    std::cerr << "[Error] send(): " << strerror(errno) << "\n";
    close(socketFd);
    throw exc::SendDataException(); // project exception
  }

  if (static_cast<std::size_t>(bytesSent) != data.size()) {
    std::cerr << "[Error] send() did not send the entire buffer\n";
    close(socketFd);
    throw exc::SendDataException();
  }
}
inline std::vector<std::uint8_t> safeRecv(int socketFd,
                                          std::size_t bufferSize) {
  std::vector<std::uint8_t> buffer(bufferSize);

  ssize_t bytesReceived = recv(socketFd, buffer.data(), buffer.size(), 0);

  if (bytesReceived == 0) {
    std::cerr << "[INFO] Client closed the connection\n";
    close(socketFd);
    throw exc::ReceiveDataException(); // project exception
  }

  if (bytesReceived < 0) {
    std::cerr << "[Error] recv(): " << strerror(errno) << "\n";
    close(socketFd);
    throw exc::ReceiveDataException(); // project exception
  }

  buffer.resize(static_cast<std::size_t>(bytesReceived));
  return buffer;
}

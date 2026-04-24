#pragma once

#include "exceptions_cpp/network_exception.h" // include your exceptions
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

inline void safeSend(int socketFd, const std::vector<std::uint8_t> &data) {
  ssize_t bytesSent = send(socketFd, data.data(), data.size(), 0);

  if (bytesSent < 0) {
    std::cerr << "[Error] send(): " << strerror(errno) << "\n";
    close(socketFd);
    throw exc::SendDataException(); // your exception
  }

  if (static_cast<std::size_t>(bytesSent) != data.size()) {
    std::cerr << "[Error] send() did not send the full buffer\n";
    close(socketFd);
    throw exc::SendDataException();
  }
}
inline std::vector<std::uint8_t> safeRecv(int socketFd, std::size_t bufferSize) {
  std::vector<std::uint8_t> buffer(bufferSize);

  ssize_t bytesReceived = recv(socketFd, buffer.data(), buffer.size(), 0);

  if (bytesReceived == 0) {
    std::cerr << "[INFO] Client closed the connection\n";
    close(socketFd);
    throw exc::ReceiveDataException(); // your exception
  }

  if (bytesReceived < 0) {
    std::cerr << "[Error] recv(): " << strerror(errno) << "\n";
    close(socketFd);
    throw exc::ReceiveDataException(); // your exception
  }

  buffer.resize(static_cast<std::size_t>(bytesReceived));
  return buffer;
}
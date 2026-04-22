#include "session_transport.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

#include "exceptions_cpp/network_exception.h"

SessionTransport::SessionTransport() : connection_(-1) {}

bool SessionTransport::IsConnected() const {
  return IsDescriptorValid(connection_);
}

int &SessionTransport::Connection() { return connection_; }

const int &SessionTransport::Connection() const { return connection_; }

bool SessionTransport::EnsureConnected(int listener_fd) {
  if (IsDescriptorValid(connection_)) {
    return false;
  }

  if (connection_ >= 0) {
    CloseInternal();
  }

  sockaddr_in client_addr{};
  socklen_t client_len = sizeof(client_addr);
  int accepted = ::accept(
      listener_fd, reinterpret_cast<sockaddr *>(&client_addr), &client_len);
  if (accepted < 0) {
    throw exc::ConnectNotAcceptException();
  }

  connection_ = accepted;
  static const char kNewConnectionMsg[] = "[Инфо] Новое соединение установлено";
  std::cout << kNewConnectionMsg << std::endl;
  return true;
}

std::vector<std::uint8_t> SessionTransport::ReceiveFrame() {
  if (!IsDescriptorValid(connection_)) {
    throw exc::SocketInvalidException();
  }

  std::uint8_t len_buf[4]{};
  std::size_t total = 0;
  while (total < sizeof(len_buf)) {
    ssize_t received =
        ::recv(connection_, len_buf + total, sizeof(len_buf) - total, 0);
    if (received <= 0) {
      throw exc::ReceiveDataException();
    }
    total += static_cast<std::size_t>(received);
  }

  uint32_t len = 0;
  std::memcpy(&len, len_buf, sizeof(len));
  len = ntohl(len);

  std::vector<std::uint8_t> buffer(len);
  std::size_t bytes_received = 0;
  while (bytes_received < buffer.size()) {
    ssize_t received = ::recv(connection_, buffer.data() + bytes_received,
                              buffer.size() - bytes_received, 0);
    if (received <= 0) {
      throw exc::ReceiveDataException();
    }
    bytes_received += static_cast<std::size_t>(received);
  }

  return buffer;
}

void SessionTransport::SendFrame(const std::vector<std::uint8_t> &data,
                                 int connection_override) {
  int fd = ResolveConnection(connection_override);
  if (!IsDescriptorValid(fd)) {
    throw exc::SocketInvalidException();
  }

  uint32_t len = htonl(static_cast<uint32_t>(data.size()));
  std::uint8_t len_buf[4];
  std::memcpy(len_buf, &len, sizeof(len));

  std::size_t total = 0;
  while (total < sizeof(len_buf)) {
    ssize_t sent = ::send(fd, len_buf + total, sizeof(len_buf) - total, 0);
    if (sent <= 0) {
      throw exc::SendDataException();
    }
    total += static_cast<std::size_t>(sent);
  }

  std::size_t bytes_sent = 0;
  while (bytes_sent < data.size()) {
    ssize_t sent =
        ::send(fd, data.data() + bytes_sent, data.size() - bytes_sent, 0);
    if (sent <= 0) {
      throw exc::SendDataException();
    }
    bytes_sent += static_cast<std::size_t>(sent);
  }
}

void SessionTransport::Reset() {
  CloseInternal();
  connection_ = -1;
}

bool SessionTransport::IsDescriptorValid(int fd) {
  return fd > 0 && ::fcntl(fd, F_GETFD) != -1;
}

int SessionTransport::ResolveConnection(int connection_override) const {
  if (connection_override >= 0) {
    return connection_override;
  }
  return connection_;
}

void SessionTransport::CloseInternal() {
  if (connection_ >= 0) {
    ::close(connection_);
  }
}

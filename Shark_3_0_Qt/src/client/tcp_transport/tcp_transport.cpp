#include "tcp_transport.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <QCoreApplication>
#include <QString>

#include "exceptions_qt/errorbus.h"
#include "exceptions_qt/exception_network.h"
#include "system/date_time_utils.h"
#include "system/serialize.h"
#include "system/system_function.h"

using exc_qt::ErrorBus;

bool TcpTransport::findServerAddress(ServerConnectionConfig &config,
                                     ServerConnectionMode &mode) {
  int socket_tmp = ::socket(AF_INET, SOCK_STREAM, 0);

  try {
    if (socket_tmp == -1) {
      throw exc_qt::CreateSocketTypeException();
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config.port);
    addr.sin_addr.s_addr = inet_addr(config.addressLocalHost.c_str());

    if (::connect(socket_tmp, reinterpret_cast<sockaddr *>(&addr),
                  sizeof(addr)) == 0) {
      config.found = true;
      mode = ServerConnectionMode::Localhost;
      ::close(socket_tmp);
      return true;
    }

    discoverServerOnLAN(config);
    if (config.found) {
      mode = ServerConnectionMode::LocalNetwork;
      ::close(socket_tmp);
      return true;
    }
  } catch (const exc_qt::CreateSocketTypeException &ex) {
    const auto time_stamp =
        formatTimeStampToString(getCurrentDateTimeInt(), true);
    const auto time_stamp_qt = QString::fromStdString(time_stamp);

    emit ErrorBus::i().error(
        QString::fromUtf8(ex.what()),
        QStringLiteral(
            "[%1]   [ERROR]   [NETWORK]   findServerAddress   search of server")
            .arg(time_stamp_qt));
  }

  if (socket_tmp >= 0) {
    ::close(socket_tmp);
  }
  mode = ServerConnectionMode::Offline;
  return false;
}

int TcpTransport::createConnection(const ServerConnectionConfig &config,
                                   ServerConnectionMode mode) {
  sockaddr_in server_address{};
  server_address.sin_port = htons(config.port);
  server_address.sin_family = AF_INET;

  switch (mode) {
    case ServerConnectionMode::Localhost: {
      server_address.sin_addr.s_addr =
          inet_addr(config.addressLocalHost.c_str());
      break;
    }
    case ServerConnectionMode::LocalNetwork: {
      addrinfo hints{};
      hints.ai_family = AF_INET;
      hints.ai_socktype = SOCK_STREAM;

      addrinfo *res = nullptr;
      int gai_result = ::getaddrinfo(config.addressLocalNetwork.c_str(),
                                     nullptr, &hints, &res);
      if (gai_result != 0 || res == nullptr) {
        server_address.sin_addr.s_addr = INADDR_NONE;
      } else {
        auto *ipv4 = reinterpret_cast<sockaddr_in *>(res->ai_addr);
        server_address.sin_addr = ipv4->sin_addr;
        ::freeaddrinfo(res);
      }
      break;
    }
    default:
      break;
  }

  int socket_tmp = ::socket(AF_INET, SOCK_STREAM, 0);

  try {
    if (socket_tmp == -1) {
      throw exc_qt::CreateSocketTypeException();
    }

    if (::connect(socket_tmp, reinterpret_cast<sockaddr *>(&server_address),
                  sizeof(server_address)) < 0) {
      throw exc_qt::ConnectionToServerException();
    }

    return socket_tmp;
  } catch (const exc_qt::NetworkException &ex) {
    const auto time_stamp =
        formatTimeStampToString(getCurrentDateTimeInt(), true);
    const auto time_stamp_qt = QString::fromStdString(time_stamp);

    emit ErrorBus::i().error(
        QString::fromUtf8(ex.what()),
        QStringLiteral(
            "[%1]   [ERROR]   [NETWORK]   createConnection   unknown mistake")
            .arg(time_stamp_qt));

    if (socket_tmp >= 0) {
      ::close(socket_tmp);
    }
    return -1;
  }
}

bool TcpTransport::discoverServerOnLAN(ServerConnectionConfig &config) {
  constexpr int timeout_ms = 1000;

  try {
    int udp_socket = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
      throw exc_qt::CreateSocketTypeException();
    }

    int broadcast_enable = 1;
    ::setsockopt(udp_socket, SOL_SOCKET, SO_BROADCAST, &broadcast_enable,
                 sizeof(broadcast_enable));

    timeval timeout{};
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    ::setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                 sizeof(timeout));

    sockaddr_in broadcast_addr{};
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(config.port);
    broadcast_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    const std::string ping{"ping?"};
    ::sendto(udp_socket, ping.c_str(), ping.size(), 0,
             reinterpret_cast<sockaddr *>(&broadcast_addr),
             sizeof(broadcast_addr));

    char buffer[128] = {0};
    sockaddr_in server_addr{};
    socklen_t addr_len = sizeof(server_addr);
    ssize_t bytes_received =
        ::recvfrom(udp_socket, buffer, sizeof(buffer) - 1, 0,
                   reinterpret_cast<sockaddr *>(&server_addr), &addr_len);

    if (bytes_received > 0) {
      std::string msg(buffer);
      if (msg == "pong") {
        config.addressLocalNetwork = inet_ntoa(server_addr.sin_addr);
        config.found = true;
      }
    }

    ::close(udp_socket);
    return true;
  } catch (const exc_qt::CreateSocketTypeException &ex) {
    const auto time_stamp =
        formatTimeStampToString(getCurrentDateTimeInt(), true);
    const auto time_stamp_qt = QString::fromStdString(time_stamp);

    emit ErrorBus::i().error(
        QString::fromUtf8(ex.what()),
        QStringLiteral(
            "[%1]   [ERROR]   [NETWORK]   discoverServerOnLAN   search in LAN")
            .arg(time_stamp_qt));
    config.found = false;
    throw;
  }
}

PacketListDTO TcpTransport::getDataFromServer(
    int socket_fd, std::atomic_bool &status_online,
    const std::vector<std::uint8_t> &payload) {
  PacketListDTO result;
  result.packets.clear();

  try {
    if (!status_online.load(std::memory_order_acquire)) {
      throw exc_qt::ConnectionToServerException();
    }

    std::vector<std::uint8_t> packet_with_size;
    uint32_t len = htonl(static_cast<uint32_t>(payload.size()));

    packet_with_size.resize(4 + payload.size());
    std::memcpy(packet_with_size.data(), &len, 4);
    std::memcpy(packet_with_size.data() + 4, payload.data(), payload.size());

    int flags = ::fcntl(socket_fd, F_GETFL, 0);
    ::fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);

    std::vector<std::uint8_t> drain_buf(4096);
    try {
      for (;;) {
        ssize_t n =
            ::recv(socket_fd, drain_buf.data(), drain_buf.size(), MSG_DONTWAIT);
        if (n > 0) {
          continue;
        }
        if (n == 0) {
          status_online.store(false, std::memory_order_release);
          throw exc_qt::ConnectionToServerException();
        }
        if (n < 0) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
            break;
          }
          if (errno == EINTR) {
            continue;
          }
          status_online.store(false, std::memory_order_release);
          throw exc_qt::ReceiveDataException();
        }
      }
    } catch (...) {
      ::fcntl(socket_fd, F_SETFL, flags);
      throw;
    }

    ::fcntl(socket_fd, F_SETFL, flags);

    std::size_t total_sent = 0;
    const std::size_t required = packet_with_size.size();

    while (total_sent < required) {
      ssize_t bytes_sent =
          ::send(socket_fd, packet_with_size.data() + total_sent,
                 required - total_sent,
#ifdef MSG_NOSIGNAL
                 MSG_NOSIGNAL
#else
                 0
#endif
          );

      if (bytes_sent > 0) {
        total_sent += static_cast<std::size_t>(bytes_sent);
        continue;
      }

      if (bytes_sent < 0) {
        if (errno == EINTR) {
          continue;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          pollfd pfd{};
          pfd.fd = socket_fd;
          pfd.events = POLLOUT;
          int pr = ::poll(&pfd, 1, 1000);
          if (pr > 0 && (pfd.revents & POLLOUT)) {
            continue;
          }
          status_online.store(false, std::memory_order_release);
          throw exc_qt::SendDataException();
        }

        if (errno == EPIPE || errno == ECONNRESET) {
          status_online.store(false, std::memory_order_release);
          throw exc_qt::ConnectionToServerException();
        }

        status_online.store(false, std::memory_order_release);
        throw exc_qt::SendDataException();
      }
    }

    len = 0;
    std::vector<PacketDTO> response_packets;

    std::uint8_t len_buf[4];
    std::size_t total = 0;
    while (total < 4) {
      ssize_t r = ::recv(socket_fd, len_buf + total, 4 - total, 0);

      if (r > 0) {
        total += static_cast<std::size_t>(r);
        continue;
      }

      if (r == 0) {
        status_online.store(false, std::memory_order_release);
        throw exc_qt::ConnectionToServerException();
      }

      if (r < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          continue;
        }
        if (errno == EINTR) {
          continue;
        }
        status_online.store(false, std::memory_order_release);
        throw exc_qt::ReceiveDataException();
      }
    }

    std::memcpy(&len, len_buf, 4);
    len = ntohl(len);

    std::vector<std::uint8_t> buffer(len);

    ssize_t bytes_received = 0;
    while (bytes_received < static_cast<ssize_t>(len)) {
      ssize_t bytes = ::recv(socket_fd, buffer.data() + bytes_received,
                             len - bytes_received, 0);

      if (bytes > 0) {
        bytes_received += bytes;
        continue;
      }

      if (bytes == 0) {
        status_online.store(false, std::memory_order_release);
        throw exc_qt::ConnectionToServerException();
      }

      if (bytes < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          continue;
        }
        if (errno == EINTR) {
          continue;
        }
        status_online.store(false, std::memory_order_release);
        throw exc_qt::ReceiveDataException();
      }
    }

    response_packets = deSerializePacketList(buffer);
    result.packets = std::move(response_packets);
  } catch (const exc_qt::NetworkException &ex) {
    const auto time_stamp =
        formatTimeStampToString(getCurrentDateTimeInt(), true);
    const auto time_stamp_qt = QString::fromStdString(time_stamp);

    emit ErrorBus::i().error(
        QString::fromUtf8(ex.what()),
        QStringLiteral("[%1]   [ERROR]   [NETWORK]   getDatafromServer   lost "
                       "connection with server")
            .arg(time_stamp_qt));
    result.packets.clear();
  }

  return result;
}

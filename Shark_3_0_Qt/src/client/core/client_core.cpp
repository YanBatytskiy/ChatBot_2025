#include "client/core/client_core.h"

#include <poll.h>
#include <unistd.h>

#include <QString>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "chat/chat.h"
#include "chat_system/chat_system.h"
#include "dto/dto_struct.h"
#include "exceptions_cpp/login_exception.h"
#include "exceptions_qt/errorbus.h"
#include "exceptions_qt/exception_login.h"
#include "exceptions_qt/exception_network.h"
#include "exceptions_qt/exception_router.h"
#include "system/date_time_utils.h"
#include "system/picosha2.h"
#include "system/serialize.h"
#include "system/system_function.h"
#include "user/user.h"
#include "user/user_chat_list.h"

#ifndef POLLRDHUP
#define POLLRDHUP 0
#endif

namespace {
constexpr int kInvalidSocket = -1;
}

ClientCore::ClientCore(ChatSystem &chat_system, QObject *parent)
    : QObject(parent), chat_system_(chat_system), transport_() {}

ClientCore::~ClientCore() = default;

bool ClientCore::getIsServerOnlineCore() const noexcept {
  return status_online_.load(std::memory_order_acquire);
}

ServerConnectionConfig &ClientCore::getServerConnectionConfigCore() {
  return server_connection_config_;
}

const ServerConnectionConfig &ClientCore::getServerConnectionConfigCore()
    const {
  return server_connection_config_;
}

ServerConnectionMode &ClientCore::getServerConnectionModeCore() {
  return server_connection_mode_;
}

const ServerConnectionMode &ClientCore::getServerConnectionModeCore() const {
  return server_connection_mode_;
}

int ClientCore::getSocketFdCore() const {
  return socket_fd_;
}

void ClientCore::setSocketFdCore(int socket_fd) {

  if (socket_fd < 0) {
    socket_fd_ = -1;
    updateConnectionStateCore(false, ServerConnectionMode::Offline);
  } else {
    socket_fd_ = socket_fd;
    updateConnectionStateCore(true, server_connection_mode_);
  }
}

bool ClientCore::findServerAddressCore(ServerConnectionConfig &config,
                                       ServerConnectionMode &mode) {
  const bool found = transport_.findServerAddress(config, mode);
  if (found) {
    server_connection_config_ = config;
    server_connection_mode_ = mode;
  }
  return found;
}

int ClientCore::createConnectionCore(const ServerConnectionConfig &config,
                                     ServerConnectionMode mode) {
  const int fd = transport_.createConnection(config, mode);
  if (fd >= 0) {
    server_connection_mode_ = mode;
    setSocketFdCore(fd);
  } else {
    updateConnectionStateCore(false, ServerConnectionMode::Offline);
  }
  return fd;
}

PacketListDTO ClientCore::processingRequestToServerCore(
    std::vector<PacketDTO> &packets, const RequestType &request_type) {
  PacketListDTO result;
  result.packets.clear();

  const int socket_fd = getSocketFdCore();
  if (socket_fd < 0 || !status_online_.load(std::memory_order_acquire)) {
    const auto time_stamp =
        formatTimeStampToString(getCurrentDateTimeInt(), true);
    const auto time_stamp_qt = QString::fromStdString(time_stamp);

    emit exc_qt::ErrorBus::i().error(
        QString::fromUtf8(""),
        QStringLiteral(
            "[%1]   [ERROR]   [NETWORK]   processingRequestToServer   no "
            "connection with server")
            .arg(time_stamp_qt));
    return result;
  }

  try {
    UserLoginPasswordDTO header;
    if (const auto &active_user = chat_system_.getActiveUser()) {
      header.login = active_user->getLogin();
    } else {
      header.login = "!";
    }
    header.passwordhash = "UserHeder";

    std::vector<PacketDTO> send_packets;
    send_packets.reserve(packets.size() + 1);

    PacketDTO header_packet;
    header_packet.requestType = request_type;
    header_packet.structDTOClassType = StructDTOClassType::userLoginPasswordDTO;
    header_packet.reqDirection = RequestDirection::ClientToSrv;
    header_packet.structDTOPtr =
        std::make_shared<StructDTOClass<UserLoginPasswordDTO>>(header);

    send_packets.push_back(std::move(header_packet));
    for (const auto &packet : packets) {
      send_packets.push_back(packet);
    }

    auto payload = serializePacketList(send_packets);
    result = transport_.getDataFromServer(socket_fd, status_online_, payload);
  } catch (const exc_qt::LostConnectionException &ex) {
    emit exc_qt::ErrorBus::i().error(
        QString::fromUtf8(ex.what()),
        QStringLiteral("Клиент processingRequestToServer: "));
    result.packets.clear();
  }

  return result;
}

void ClientCore::updateConnectionStateCore(bool online,
                                           ServerConnectionMode mode) {
  const bool previous =
      status_online_.exchange(online, std::memory_order_acq_rel);
  chat_system_.setIsServerStatus(online);

  if (previous != online || server_connection_mode_ != mode) {
    server_connection_mode_ = mode;
    emit serverStatusChanged(online, mode);
  }
}

// === monitoring ===
void ClientCore::connectionMonitorLoopCore(std::atomic_bool &running_flag) {
  bool online = getIsServerOnlineCore();

  while (running_flag.load(std::memory_order_acquire)) {
    if (!online) {
      auto &config = getServerConnectionConfigCore();
      auto &mode_ref = getServerConnectionModeCore();

      if (!findServerAddressCore(config, mode_ref)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        continue;
      }

      const int fd = createConnectionCore(config, mode_ref);

      if (fd < 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        continue;
      }

      online = true;
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      continue;
    }

    const int fd = getSocketFdCore();
    if (!socketAliveCore(fd)) {
      if (fd >= 0) {
        ::close(fd);
      }
      setSocketFdCore(-1);
      online = false;
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      continue;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}

bool ClientCore::socketAliveCore(int fd) {
  if (fd < 0) {
    return false;
  }

  pollfd descriptor{};
  descriptor.fd = fd;
  descriptor.events = POLLIN | POLLERR | POLLHUP | POLLRDHUP;

  const int result = ::poll(&descriptor, 1, 0);

  if (result < 0) {
    return false;
  }

  if (result == 0) {
    return true;
  }

  if (descriptor.revents & (POLLERR | POLLHUP | POLLRDHUP)) {
    return false;
  }

  return true;
}

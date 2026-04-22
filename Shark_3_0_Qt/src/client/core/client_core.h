#pragma once

#include <QObject>
#include <atomic>

#include "client/tcp_transport/session_types.h"
#include "client/tcp_transport/tcp_transport.h"
#include "dto_struct.h"

class ChatSystem;
class Chat;
class User;

class ClientCore : public QObject {
  Q_OBJECT
 public:
  ClientCore(ChatSystem &chat_system, QObject *parent = nullptr);
  ClientCore(const ClientCore &) = delete;
  ClientCore &operator=(const ClientCore &) = delete;
  ClientCore(ClientCore &&) = delete;
  ClientCore &operator=(ClientCore &&) = delete;

  ~ClientCore();

  // getters
  bool getIsServerOnlineCore() const noexcept;

  ServerConnectionConfig &getServerConnectionConfigCore();
  const ServerConnectionConfig &getServerConnectionConfigCore() const;
  ServerConnectionMode &getServerConnectionModeCore();
  const ServerConnectionMode &getServerConnectionModeCore() const;
  int getSocketFdCore() const;

  // setters
  void setSocketFdCore(int socket_fd);

  // transport
  bool findServerAddressCore(ServerConnectionConfig &config,
                             ServerConnectionMode &mode);

  int createConnectionCore(const ServerConnectionConfig &config,
                           ServerConnectionMode mode);

  void connectionMonitorLoopCore(std::atomic_bool &running_flag);
  static bool socketAliveCore(int fd);

  // utilities
  PacketListDTO processingRequestToServerCore(std::vector<PacketDTO> &packets,
                                              const RequestType &request_type);

 signals:
  void serverStatusChanged(bool online, ServerConnectionMode mode);

 private:
  void updateConnectionStateCore(bool online, ServerConnectionMode mode);

  ChatSystem &chat_system_;
  TcpTransport transport_;
  std::atomic_bool status_online_{false};
  ServerConnectionConfig server_connection_config_{};
  ServerConnectionMode server_connection_mode_{ServerConnectionMode::Offline};

  int socket_fd_{-1};
};

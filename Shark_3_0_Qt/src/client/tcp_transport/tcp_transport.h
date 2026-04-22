#pragma once

#include "session_types.h"
#include "dto_struct.h"

#include <atomic>
#include <cstdint>
#include <vector>

class TcpTransport {
public:
  TcpTransport() = default;

  bool findServerAddress(ServerConnectionConfig &config, ServerConnectionMode &mode);
  int createConnection(const ServerConnectionConfig &config, ServerConnectionMode mode);
  bool discoverServerOnLAN(ServerConnectionConfig &config);
  PacketListDTO getDataFromServer(int socket_fd, std::atomic_bool &status_online,
                                  const std::vector<std::uint8_t> &payload);
};

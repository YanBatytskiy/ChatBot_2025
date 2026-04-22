#pragma once

#include <cstdint>
#include <string>

enum class ServerConnectionMode {
  Localhost,
  LocalNetwork,
  Offline
};

struct ServerConnectionConfig {
  std::string addressLocalHost{"127.0.0.1"};
  std::string addressLocalNetwork{};
  std::uint16_t port{50000};
  bool found{false};
};


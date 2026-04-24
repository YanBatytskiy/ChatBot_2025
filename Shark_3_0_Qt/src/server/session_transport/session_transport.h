#pragma once

#include <cstdint>
#include <vector>

class SessionTransport {
public:
  SessionTransport();

  bool IsConnected() const;

  int &Connection();
  const int &Connection() const;

  bool EnsureConnected(int listener_fd);

  std::vector<std::uint8_t> ReceiveFrame();

  void SendFrame(const std::vector<std::uint8_t> &data,
                 int connection_override = -1);

  void Reset();

private:
  static bool IsDescriptorValid(int fd);

  int ResolveConnection(int connection_override) const;

  void CloseInternal();

  int connection_;
};

#ifndef NW_CONNECTION_MONITOR_H
#define NW_CONNECTION_MONITOR_H

#include "client/tcp_transport/session_types.h"

#include <QObject>
#include <atomic>
#include <poll.h>

class ClientCore;

#ifndef POLLRDHUP
#define POLLRDHUP 0
#endif

class ConnectionMonitor : public QObject {
  Q_OBJECT
public:
  explicit ConnectionMonitor(ClientCore *core) : core_(core) {}

public slots:
  void run();
  void stop() { run_.store(false, std::memory_order_release); }

signals:
  void connectionStateChanged(bool connectionStatus, ServerConnectionMode serverConnectionMode);

private:
  ClientCore *core_;
  std::atomic_bool run_{true};

  static bool socketAlive(int fd) {
    if (fd < 0)
      return false;
    pollfd p{};
    p.fd = fd;
    p.events = POLLIN | POLLERR | POLLHUP | POLLRDHUP;

    const int result = ::poll(&p, 1, 0);

    if (result < 0)
      return false;
    if (result == 0)
      return true;

    if (p.revents & (POLLERR | POLLHUP | POLLRDHUP))
      return false;
    return true;
  }
};

#endif // NW_CONNECTION_MONITOR_H

#include "nw_connection_monitor.h"

#include "client/core/client_core.h"

#include <chrono>
#include <thread>
#include <unistd.h>

void ConnectionMonitor::run() {
  try {
    bool online = false;
    ServerConnectionMode mode = ServerConnectionMode::Offline;

    while (run_.load(std::memory_order_acquire)) {
      if (!online) {
        if (!core_->findServerAddressCore(core_->getServerConnectionConfigCore(), core_->getServerConnectionModeCore())) {
          std::this_thread::sleep_for(std::chrono::milliseconds(500));
          continue;
        }

        mode = core_->getServerConnectionModeCore();
        const int fd = core_->createConnectionCore(core_->getServerConnectionConfigCore(), mode);

        if (fd < 0) {
          std::this_thread::sleep_for(std::chrono::milliseconds(500));
          continue;
        }

        core_->setSocketFdCore(fd);
        online = true;
        emit connectionStateChanged(true, mode);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        continue;
      }

      const int fd = core_->getSocketFdCore();
      const bool alive = socketAlive(fd);

      if (!alive) {
        online = false;
        mode = ServerConnectionMode::Offline;
        if (fd >= 0)
          ::close(fd);
        core_->setSocketFdCore(-1);
        emit connectionStateChanged(false, mode);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        continue;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  } catch (const std::exception &) {
    return;
  }
}

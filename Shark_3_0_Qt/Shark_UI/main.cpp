#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include <QApplication>
#include <QMessageBox>
#include <QObject>
#include <QStyleFactory>
#include <memory>

#include "chat_system/chat_system.h"
#include "client/client_session.h"
#include "errorbus.h"
#include "logger.h"
#include "screen_main_window.h"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  ChatSystem chat_system;
  auto client_session_ptr = std::make_shared<ClientSession>(chat_system);
  auto logger_ptr = std::make_shared<Logger>();

  (void)exc_qt::ErrorBus::i();

  QObject::connect(&exc_qt::ErrorBus::i(), &exc_qt::ErrorBus::error, &app,
                   [logger_ptr](const QString &m, const QString &ctx) {
                     QMessageBox::critical(qApp->activeWindow(), "Ошибка",
                                           QString("[%1]\n%2").arg(ctx, m));

                     // записать в лог
                     QString log_line = QStringLiteral("%1   %2").arg(ctx, m);
                     emit logger_ptr->signalWriteLine(log_line);
                   });

  client_session_ptr->startConnectionThreadCl();

  QObject::connect(&app, &QCoreApplication::aboutToQuit,
                   [&] { client_session_ptr->stopConnectionThreadCl(); });

  QObject::connect(&app, &QCoreApplication::aboutToQuit, logger_ptr.get(),
                   &Logger::slotStopLogger);

  auto w = new MainWindow(client_session_ptr, logger_ptr);
  w->show();
  auto result = app.exec();

  if (result == QDialog::Rejected) return 0;

  return 0;
}

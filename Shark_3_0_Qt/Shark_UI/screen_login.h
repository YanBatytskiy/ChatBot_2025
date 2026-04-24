#ifndef LOGINSCREEN_H
#define LOGINSCREEN_H

#include "client_session.h"
#include "logger.h"
#include "screen_logger_form.h"
#include <QWidget>

class ConnectionMonitor;

namespace Ui {
class ScreenLogin;
}

class ScreenLogin : public QWidget {
  Q_OBJECT
public:
  explicit ScreenLogin(QWidget *parent = nullptr);
  ~ScreenLogin();
  void setDatabase(std::shared_ptr<ClientSession> client_session_ptr, std::shared_ptr<Logger> logger_ptr);
  void clearFields();

signals:
  void signal_registration_requested();
  void accepted(QString login);
  void rejected();

public slots:
  void onConnectionStatusChanged(bool connectionStatus,
                                 ServerConnectionMode mode);

  void slot_show_logger_form(const std::multimap<qint64, QString> &logger_model);

private slots:
  void slot_on_register_mode_button_clicked();
  void on_loginButtonBox_accepted();
  void on_loginButtonBox_rejected();

  void checkSignIn();

  void on_loginEdit_returnPressed();
  void on_passwordEdit_returnPressed();

  void on_baseReInitialisationPushButton_clicked();

  void slotOn_logFileClearPushButton_clicked();
  void slotOn_lookLogSeveralLinePushButton_clicked();
  void slotOn_lookLogLastLinePushButton_clicked();

private:
  Ui::ScreenLogin *ui;
  ScreenLoggerForm *logger_form_{nullptr};
  std::shared_ptr<ClientSession> client_session_ptr_;
  std::shared_ptr<Logger> logger_ptr_;
  QString _systemData;
};

#endif // LOGINSCREEN_H

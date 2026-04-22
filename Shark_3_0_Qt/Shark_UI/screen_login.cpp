#include "screen_login.h"

#include <sys/utsname.h>

#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>

#include "exceptions_qt/errorbus.h"
#include "logger.h"
#include "system/date_time_utils.h"
#include "ui_screen_login.h"

ScreenLogin::ScreenLogin(QWidget *parent)
    : QWidget(parent), ui(new Ui::ScreenLogin) {
  ui->setupUi(this);

  connect(ui->logFileClearPushButton, &QPushButton::clicked, this,
          &ScreenLogin::slotOn_logFileClearPushButton_clicked);

  connect(ui->lookLogSeveralLinePushButton, &QPushButton::clicked, this,
          &ScreenLogin::slotOn_lookLogSeveralLinePushButton_clicked);

  connect(ui->lookLogLastLinePushButton, &QPushButton::clicked, this,
          &ScreenLogin::slotOn_lookLogLastLinePushButton_clicked);

  connect(ui->registerModeButton, &QPushButton::clicked, this,
          &ScreenLogin::slot_on_register_mode_button_clicked);
}

ScreenLogin::~ScreenLogin() { delete ui; }

void ScreenLogin::setDatabase(std::shared_ptr<ClientSession> client_session_ptr,
                              std::shared_ptr<Logger> logger_ptr) {
  client_session_ptr_ = client_session_ptr;
  logger_ptr_ = logger_ptr;

  connect(client_session_ptr_.get(), &ClientSession::serverStatusChanged, this,
          &ScreenLogin::onConnectionStatusChanged, Qt::QueuedConnection);

  struct utsname utsname;
  uname(&utsname);

  _systemData = "OS name: " + QString::fromStdString(utsname.sysname) +
                "\nOS Release: " + QString::fromStdString(utsname.release) +
                "\nOS version: " + QString::fromStdString(utsname.version) +
                "\nArchitecture: " + QString::fromStdString(utsname.machine) +
                "\nHost name: " + QString::fromStdString(utsname.nodename);

  QString serverDataText;

  ServerConnectionConfig serverConnectionConfig;
  serverDataText = _systemData;

  auto mode = client_session_ptr_->getserverConnectionModeCl();

  if (mode == ServerConnectionMode::Localhost) {
    serverDataText += "\n\nПодключено к серверу внутри рабочей станции.";
    const auto time_stamp = QString::fromStdString(
        formatTimeStampToString(getCurrentDateTimeInt(), true));
    const QString event =
        "Connection to Server established to local host " +
        QString::fromStdString(serverConnectionConfig.addressLocalHost) +
        "port: " + QString::number(serverConnectionConfig.port);

    const QString log_line = QStringLiteral("[%1]   [%2]   [%3]   %4")
                                 .arg(time_stamp,
                                      "INFO",  // Уровни логгирования:
                                      "NETWORK",  // Ключевые модули для чата
                                      event);  // Event

    emit logger_ptr_->signalWriteLine(log_line);

  } else if (mode == ServerConnectionMode::LocalNetwork) {
    serverDataText += "\n\nПодключено к серверу внути локальной сети.";
    const auto time_stamp = QString::fromStdString(
        formatTimeStampToString(getCurrentDateTimeInt(), true));
    const QString event =
        "Connection to Server established to local network " +
        QString::fromStdString(serverConnectionConfig.addressLocalNetwork) +
        "port: " + QString::number(serverConnectionConfig.port);

    const QString log_line = QStringLiteral("[%1]   [%2]   [%3]   %4")
                                 .arg(time_stamp,
                                      "INFO",  // Уровни логгирования:
                                      "NETWORK",  // Ключевые модули для чата
                                      event);  // Event

    emit logger_ptr_->signalWriteLine(log_line);
  } else if (mode == ServerConnectionMode::Offline)
    serverDataText += "\n\nПодключение к серверу не удалось. Режим Offline.";

  if (mode == ServerConnectionMode::Localhost) {
    serverDataText += QString::fromStdString("\nLocalHost address: ");
    serverDataText +=
        QString::fromStdString(serverConnectionConfig.addressLocalHost);
    serverDataText += QString::fromStdString("\nPort: ");
    serverDataText += QString::number(serverConnectionConfig.port);
  } else if (mode == ServerConnectionMode::LocalNetwork) {
    serverDataText += QString::fromStdString("\nLocal Network Address: ");
    serverDataText +=
        QString::fromStdString(serverConnectionConfig.addressLocalNetwork);
    serverDataText += QString::fromStdString("\nPort: ");
    serverDataText += QString::number(serverConnectionConfig.port);
  };

  ui->serverDataLabel->setText(serverDataText);
}

void ScreenLogin::clearFields() {
  ui->loginEdit->clear();
  ui->passwordEdit->clear();
  ui->loginEdit->setFocus();
}

void ScreenLogin::onConnectionStatusChanged(bool connectionStatus,
                                            ServerConnectionMode mode) {
  QString serverDataText;

  ServerConnectionConfig serverConnectionConfig;
  serverDataText = _systemData;

  if (mode == ServerConnectionMode::Localhost) {
    serverDataText += "\n\nПодключено к серверу внутри рабочей станции.";

    const auto time_stamp = QString::fromStdString(
        formatTimeStampToString(getCurrentDateTimeInt(), true));
    const QString event =
        "Connection to Server established to local host " +
        QString::fromStdString(serverConnectionConfig.addressLocalHost) +
        "port: " + QString::number(serverConnectionConfig.port);

    const QString log_line = QStringLiteral("[%1]   [%2]   [%3]   %4")
                                 .arg(time_stamp,
                                      "INFO",  // Уровни логгирования:
                                      "NETWORK",  // Ключевые модули для чата
                                      event);  // Event

    emit logger_ptr_->signalWriteLine(log_line);

  } else if (mode == ServerConnectionMode::LocalNetwork) {
    serverDataText += "\n\nПодключено к серверу внути локальной сети.";

    const auto time_stamp = QString::fromStdString(
        formatTimeStampToString(getCurrentDateTimeInt(), true));
    const QString event =
        "Connection to Server established to local network " +
        QString::fromStdString(serverConnectionConfig.addressLocalNetwork) +
        "port: " + QString::number(serverConnectionConfig.port);

    const QString log_line = QStringLiteral("[%1]   [%2]   [%3]   %4")
                                 .arg(time_stamp,
                                      "INFO",  // Уровни логгирования:
                                      "NETWORK",  // Ключевые модули для чата
                                      event);  // Event

    emit logger_ptr_->signalWriteLine(log_line);

  } else if (mode == ServerConnectionMode::Offline) {
    const auto time_stamp = QString::fromStdString(
        formatTimeStampToString(getCurrentDateTimeInt(), true));
    const QString event = "Connection to Server lost";
    const QString log_line = QStringLiteral("[%1]   [%2]   [%3]   %4")
                                 .arg(time_stamp,
                                      "ERROR",  // Уровни логгирования:
                                      "NETWORK",  // Ключевые модули для чата
                                      event);  // Event

    emit logger_ptr_->signalWriteLine(log_line);

    serverDataText += "\n\nПодключение к серверу не удалось. Режим Offline.";
  }

  if (mode == ServerConnectionMode::Localhost) {
    serverDataText += QString::fromStdString("\nLocalHost address: ");
    serverDataText +=
        QString::fromStdString(serverConnectionConfig.addressLocalHost);
    serverDataText += QString::fromStdString("\nPort: ");
    serverDataText += QString::number(serverConnectionConfig.port);
  } else if (mode == ServerConnectionMode::LocalNetwork) {
    serverDataText += QString::fromStdString("\nLocal Network Address: ");
    serverDataText +=
        QString::fromStdString(serverConnectionConfig.addressLocalNetwork);
    serverDataText += QString::fromStdString("\nPort: ");
    serverDataText += QString::number(serverConnectionConfig.port);
  };

  ui->serverDataLabel->setText(serverDataText);

  if (connectionStatus) {
    ui->serverStatusLabelRound->setStyleSheet(
        "background-color: green; border-radius: 8px;");
    ui->serverStatusLabel->setText("server online");
  } else {
    ui->serverStatusLabelRound->setStyleSheet(
        "background-color: red; border-radius: 8px;");
    ui->serverStatusLabel->setText("server offline");
  }
}

void ScreenLogin::slot_show_logger_form(
    const std::multimap<qint64, QString> &logger_model) {
  if (logger_form_ == nullptr)
    logger_form_ = new ScreenLoggerForm(client_session_ptr_, logger_ptr_, this);

  logger_form_->slot_fill_logger_model(logger_model);

  logger_form_->setWindowModality(Qt::ApplicationModal);
  logger_form_->setWindowFlags(Qt::Dialog | Qt::WindowMinMaxButtonsHint);
  logger_form_->show();
}

void ScreenLogin::slot_on_register_mode_button_clicked() {
  if (!client_session_ptr_->getIsServerOnlineCl()) {
    QMessageBox::warning(this, tr("No!"), tr("Сервер не доступен."));
    return;
  }
  clearFields();
  emit signal_registration_requested();
}

void ScreenLogin::on_loginButtonBox_accepted() { checkSignIn(); }

void ScreenLogin::on_loginButtonBox_rejected() { emit rejected(); }

void ScreenLogin::checkSignIn() {
  try {
    const auto newInput = ui->passwordEdit->text().toStdString();

    if (ui->loginEdit->text().toStdString() == "" || newInput == "") return;

    if (!client_session_ptr_->getIsServerOnlineCl()) {
      auto time_stamp = QString::fromStdString(
          formatTimeStampToString(getCurrentDateTimeInt(), true));
      QString event = "Server unavailable";
      QString user_login = ui->loginEdit->text();

      QString log_line = QStringLiteral("[%1]   [%2]   [%3]   [user=%4]   %5")
                             .arg(time_stamp,
                                  "ERROR",  // Уровни логгирования:
                                  "NETWORK",  // Ключевые модули для чата
                                  user_login,  // User
                                  event);      // Event

      emit logger_ptr_->signalWriteLine(log_line);

      QMessageBox::warning(this, tr("No!"), tr("Сервер не доступен."));
      return;
    }

    auto login = ui->loginEdit->text().toStdString();
    auto password = ui->passwordEdit->text().toStdString();

    auto result = client_session_ptr_->checkUserPasswordCl(login, password);

    if (!result) {
      auto time_stamp = QString::fromStdString(
          formatTimeStampToString(getCurrentDateTimeInt(), true));
      QString event = "Authentication failed: invalid token";
      QString user_login = ui->loginEdit->text();

      QString log_line = QStringLiteral("[%1]   [%2]   [%3]   [user=%4]   %5")
                             .arg(time_stamp,
                                  "ERROR",  // Уровни логгирования:
                                  "AUTH",  // Ключевые модули для чата
                                  user_login,  // User
                                  event);      // Event

      emit logger_ptr_->signalWriteLine(log_line);
      emit exc_qt::ErrorBus::i().error(tr("Login or Password is wrong"),
                                       "login");
      return;
    }

    auto time_stamp = QString::fromStdString(
        formatTimeStampToString(getCurrentDateTimeInt(), true));
    QString event = "Authentification sucsessed";
    QString user_login = ui->loginEdit->text();

    QString log_line = QStringLiteral("[%1]   [%2]   [%3]   [user=%4]   %5")
                           .arg(time_stamp,
                                "INFO",  // Уровни логгирования:
                                "AUTH",  // Ключевые модули для чата
                                user_login,  // User
                                event);      // Event

    emit logger_ptr_->signalWriteLine(log_line);

    emit accepted(ui->loginEdit->text());

    //     event = "Connection to Server established to local network " +
    //             QString::fromStdString(
    //                 client_session_ptr_->getserverConnectionConfigCl()
    //                     .addressLocalNetwork) +
    //             "port: " +
    //             QString::number(
    //                 client_session_ptr_->getserverConnectionConfigCl().port);

    //     log_line = QStringLiteral("[%1]   [%2]   [%3]   %4")
    //                    .arg(time_stamp,
    //                         "INFO",    // Уровни логгирования:
    //                         "NETWORK", // Ключевые модули для чата
    //                         event);    // Event

    //     emit logger_ptr_->signalWriteLine(log_line);

    //   else if (mode == ServerConnectionMode::Offline) {
    //     auto time_stamp = QString::fromStdString(
    //         formatTimeStampToString(getCurrentDateTimeInt(), true));
    //     QString event = "Connection to Server lost";
    //     QString log_line = QStringLiteral("[%1]   [%2]   [%3]   %4")
    //                                  .arg(time_stamp,
    //                                       "ERROR", // Уровни логгирования:
    //                                       "NETWORK", // Ключевые модули для
    //                                       чата
    //   }
  } catch (const std::exception &) {
    return;
  }
}

void ScreenLogin::on_loginEdit_returnPressed() {
  if (ui->loginEdit->text().toStdString() != "") ui->passwordEdit->setFocus();
}

void ScreenLogin::on_passwordEdit_returnPressed() {
  if (ui->loginEdit->text().toStdString() == "" ||
      ui->passwordEdit->text().toStdString() == "")
    return;
  checkSignIn();
}

void ScreenLogin::on_baseReInitialisationPushButton_clicked() {
  bool result = client_session_ptr_->reInitilizeBaseCl();

  if (!result) {
    const auto time_stamp = QString::fromStdString(
        formatTimeStampToString(getCurrentDateTimeInt(), true));
    const QString event = "Base reinitialisation failed";
    const QString user_login = "";

    const QString log_line =
        QStringLiteral("[%1]   [%2]   [%3]   [user=%4]   %5")
            .arg(time_stamp,
                 "ERROR",     // Уровни логгирования:
                 "DATABASE",  // Ключевые модули для чата
                 user_login,  // User
                 event);      // Event

    emit logger_ptr_->signalWriteLine(log_line);

    QMessageBox::warning(this, "Ошибка",
                         "Не удалось переинициализировать базу.");
  } else {
    const auto time_stamp = QString::fromStdString(
        formatTimeStampToString(getCurrentDateTimeInt(), true));
    const QString event = "Base reinitialisation sucsessed";
    const QString user_login = "";

    const QString log_line =
        QStringLiteral("[%1]   [%2]   [%3]   [user=%4]   %5")
            .arg(time_stamp,
                 "INFO",      // Уровни логгирования:
                 "DATABASE",  // Ключевые модули для чата
                 user_login,  // User
                 event);      // Event

    emit logger_ptr_->signalWriteLine(log_line);

    QMessageBox::information(this, "Успешно.", "Можно входить");
  }
}

void ScreenLogin::slotOn_logFileClearPushButton_clicked() {
  QMessageBox message(this);
  message.setWindowTitle("Подтверждение.");
  message.setText("Вы уверены, что хотите очистить файл логов?");
  auto clearButton = message.addButton("Очистить", QMessageBox::AcceptRole);
  message.addButton("Отмена", QMessageBox::RejectRole);

  message.exec();

  const auto clicked = message.clickedButton();
  if (clicked != clearButton) return;

  if (!logger_ptr_) {
    QMessageBox::warning(this, "Ошибка", "Логгер недоступен.");
    return;
  }

  const bool cleared = logger_ptr_->slotClearLogFile();
  if (cleared)
    QMessageBox::information(this, "Сообщение", "Файл логов очищен.");
  else
    QMessageBox::warning(this, "Ошибка", "Ошибка доступа к файлу.");
}

void ScreenLogin::slotOn_lookLogSeveralLinePushButton_clicked() {
  QMessageBox message(this);
  message.setWindowTitle("Выберите сколько строк выводить.");
  message.setText(
      "Выберите некоторое количиство последних строк либо весь лог");
  message.addButton("Ввести количество строк", QMessageBox::AcceptRole);
  message.addButton("Вывести все строки", QMessageBox::AcceptRole);
  message.addButton("Отмена", QMessageBox::RejectRole);

  message.exec();

  if (message.clickedButton()->text() == "Ввести количество строк") {
    bool ok;
    int count = QInputDialog::getInt(
        this, "Количество:", "Ведите количество:", 10, 1, 1000, 1, &ok);
    if (ok) {
      const auto &model_data = logger_ptr_->slotReadSeveralLines(count);
      slot_show_logger_form(model_data);
    }
  } else if (message.clickedButton()->text() == "Вывести все строки") {
    const auto &model_data = logger_ptr_->slotReadSeveralLines(0);
    slot_show_logger_form(model_data);
  }
}

void ScreenLogin::slotOn_lookLogLastLinePushButton_clicked() {
  const auto &model_data = logger_ptr_->slotReadLastLine();

  slot_show_logger_form(model_data);
}

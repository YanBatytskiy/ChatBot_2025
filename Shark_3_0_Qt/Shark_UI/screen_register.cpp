#include "screen_register.h"

#include <QMessageBox>

#include "system/picosha2.h"
#include "system/system_function.h"
#include "ui_screen_register.h"

ScreenRegister::ScreenRegister(QWidget *parent)
    : QWidget(parent), ui(new Ui::ScreenRegister) {
  ui->setupUi(this);

  ui->loginEdit->installEventFilter(this);
  ui->nameEdit->installEventFilter(this);
  ui->emailLineEdit->installEventFilter(this);
  ui->phoneLineEdit->installEventFilter(this);
  ui->passwordEdit->installEventFilter(this);
  ui->passwordConfirmEdit->installEventFilter(this);
}

bool ScreenRegister::eventFilter(QObject *watched, QEvent *event) {
  const bool isEdit = watched == ui->loginEdit || watched == ui->nameEdit ||
                      watched == ui->passwordEdit ||
                      watched == ui->passwordConfirmEdit;

  if (isEdit) {
    if (event->type() == QEvent::FocusIn) {
      if (watched == ui->loginEdit || watched == ui->passwordEdit ||
          watched == ui->passwordConfirmEdit) {
        ui->hintLabel->setText(
            "Не менее 5 и не более 20 символов, только английские буквы и "
            "цифры");
      } else if (watched == ui->nameEdit) {
        ui->hintLabel->setText(
            "Не менее 2 и не более 20 символов, только английские буквы и "
            "цифры");
      } else if (watched == ui->emailLineEdit || watched == ui->phoneLineEdit) {
        ui->hintLabel->clear();
      }
    }
    return false;
  }

  if (event->type() == QEvent::FocusOut) {
    ui->hintLabel->clear();
    return false;
  }

  return QWidget::eventFilter(watched, event);
}

ScreenRegister::~ScreenRegister() { delete ui; }

void ScreenRegister::setDatabase(
    std::shared_ptr<ClientSession> client_session_ptr,
    std::shared_ptr<Logger> logger_ptr) {
  client_session_ptr_ = client_session_ptr;
  logger_ptr_ = logger_ptr;

  connect(client_session_ptr_.get(), &ClientSession::serverStatusChanged, this,
          &ScreenRegister::onConnectionStatusChanged, Qt::QueuedConnection);
}

void ScreenRegister::clearFields() {
  ui->loginEdit->clear();
  ui->nameEdit->clear();
  ui->emailLineEdit->clear();
  ui->phoneLineEdit->clear();
  ui->passwordEdit->clear();
  ui->passwordConfirmEdit->clear();
  ui->loginEdit->setFocus();
}

void ScreenRegister::onConnectionStatusChanged(bool connectionStatus,
                                               ServerConnectionMode mode) {
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

void ScreenRegister::on_toLoginButton_clicked() {
  clearFields();
  emit loginRequested();
}

void ScreenRegister::on_loginEdit_editingFinished() {
  if (ui->loginEdit->text().size() == 0) return;

  if (ui->loginEdit->text().size() < 5 || ui->loginEdit->text().size() > 20) {
    QMessageBox::critical(
        this, "Ошибка",
        "Некорректная длина логина. Длина должна быть от 5 до 20 символов.");
    ui->loginEdit->setStyleSheet("QLineEdit { color: red; }");
    isLogin = false;
    return;
  }

  if (!engAndFiguresCheck(ui->loginEdit->text().toStdString())) {
    ui->loginEdit->setStyleSheet("QLineEdit { color: red; }");
    isLogin = false;
  } else {
    ui->loginEdit->setStyleSheet("QLineEdit { color: black; }");
    isLogin = true;
  }
}

void ScreenRegister::on_nameEdit_editingFinished() {
  if (ui->nameEdit->text().size() == 0) return;

  if (ui->nameEdit->text().size() < 2 || ui->nameEdit->text().size() > 20) {
    QMessageBox::critical(
        this, "Ошибка",
        "Некорректная длина имени. Длина должна быть от 2 до 20 символов.");
    ui->nameEdit->setStyleSheet("QLineEdit { color: red; }");
    isName = false;
    return;
  }

  if (!engAndFiguresCheck(ui->nameEdit->text().toStdString())) {
    QMessageBox::critical(
        this, "Ошибка",
        "Недопустимое имя. Наведи мышку на поле для полсказки.");
    ui->nameEdit->setStyleSheet("QLineEdit { color: red; }");
    isName = false;
  } else {
    ui->nameEdit->setStyleSheet("QLineEdit { color: black; }");
    isName = true;
  }
}

void ScreenRegister::on_passwordEdit_editingFinished() {
  if (ui->passwordEdit->text().size() == 0) return;

  if (ui->passwordEdit->text().size() < 5 ||
      ui->passwordEdit->text().size() > 20) {
    QMessageBox::critical(
        this, "Ошибка",
        "Некорректная длина пароля. Длина должна быть от 5 до 20 символов.");
    ui->passwordEdit->setStyleSheet("QLineEdit { color: red; }");
    isPassword = false;
    return;
  }

  if (!checkNewLoginPasswordForLimits(ui->passwordEdit->text().toStdString(), 5,
                                      20, true)) {
    QMessageBox::critical(
        this, "Ошибка",
        "Недопустимый пароль. Наведи мышку на поле для полсказки.");
    ui->passwordEdit->setStyleSheet("QLineEdit { color: red; }");
  } else
    ui->passwordEdit->setStyleSheet("QLineEdit { color: black; }");

  isPassword = false;
  ui->passwordConfirmEdit->text() = "";
}

void ScreenRegister::on_passwordConfirmEdit_editingFinished() {
  if (ui->passwordEdit->text() != ui->passwordConfirmEdit->text()) {
    QMessageBox::critical(this, "Ошибка", "Пароли не совпадают.");
    ui->passwordConfirmEdit->setStyleSheet("QLineEdit { color: red; }");
    isPassword = false;
  } else {
    ui->passwordConfirmEdit->setStyleSheet("QLineEdit { color: black; }");
    isPassword = true;
  }
}

void ScreenRegister::on_exitPushButton_clicked() {
  clearFields();
  emit rejected();
}

void ScreenRegister::on_registerPushButton_clicked() {
  if (!isLogin || !isName || !isPassword) {
    QMessageBox::critical(this, "Ошибка",
                          "Поля логин, имя и пароль должны быть заполнены.");
    return;
  }
  const auto &login = ui->loginEdit->text().toStdString();

  if (client_session_ptr_->checkUserLoginCl(login))
    QMessageBox::critical(this, "Ошибка", "Такой логин уже существует.");
  else {
    UserDTO userDTO;

    userDTO.login = ui->loginEdit->text().toStdString();
    userDTO.userName = ui->nameEdit->text().toStdString();
    userDTO.email = ui->emailLineEdit->text().toStdString();
    userDTO.phone = ui->phoneLineEdit->text().toStdString();

    const auto passHash =
        picosha2::hash256_hex_string(ui->passwordEdit->text().toStdString());
    userDTO.passwordhash = passHash;
    userDTO.ban_until = 0;
    userDTO.disable_reason = "";
    userDTO.is_active = true;
    userDTO.disabled_at = 0;

    if (client_session_ptr_->createUserCl(userDTO)) {
      auto user_ptr = std::make_shared<User>(
          UserData(userDTO.login, userDTO.userName, userDTO.passwordhash,
                   userDTO.email, userDTO.phone, "", true, 0, 0));

      client_session_ptr_->getInstanceCl().addUserToSystem(user_ptr);

    }  // if create
    else {
      QMessageBox::critical(
          this, "Ошибка", "Регистрация не удалась. Попробуйте позже еще раз.");
    }

    emit signalLoggedIn(ui->loginEdit->text());

  }  // if else checkLogin
}

void ScreenRegister::on_passwordConfirmEdit_cursorPositionChanged(int arg1,
                                                                  int arg2) {}

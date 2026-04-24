#include "screen_user_profile.h"

#include <QMessageBox>

#include "dto/dto_struct.h"
#include "system/picosha2.h"
#include "system/system_function.h"
#include "ui_screen_user_profile.h"

ScreenUserProfile::ScreenUserProfile(QWidget *parent)
    : QWidget(parent), ui(new Ui::ScreenUserProfile) {
  ui->setupUi(this);
  _userData._name = "";
  _userData._email = "";
  _userData._phone = "";

  ui->nameLineEdit->installEventFilter(this);
  ui->emailLineEdit->installEventFilter(this);
  ui->phoneLineEdit->installEventFilter(this);
  ui->passwordLineEdit->installEventFilter(this);
  ui->confirnPasswordLineEdit->installEventFilter(this);
}

bool ScreenUserProfile::eventFilter(QObject *watched, QEvent *event) {
  const bool isEdit =
    watched == ui->nameLineEdit || watched == ui->emailLineEdit ||
    watched == ui->phoneLineEdit || watched == ui->passwordLineEdit ||
    watched == ui->confirnPasswordLineEdit;

  if (isEdit) {
    if (event->type() == QEvent::FocusIn) {
      if (watched == ui->passwordLineEdit ||
          watched == ui->confirnPasswordLineEdit) {
        ui->hintLabel->setText(
          "At least 5 and no more than 20 characters, English letters "
          "and digits only");
      } else if (watched == ui->nameLineEdit) {
        ui->hintLabel->setText(
          "At least 2 and no more than 20 characters, English letters "
          "and digits only");
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

ScreenUserProfile::~ScreenUserProfile() {
  delete ui;
}

void ScreenUserProfile::setDatabase(
  std::shared_ptr<ClientSession> client_session_ptr) {
  client_session_ptr_ = client_session_ptr;
}

void ScreenUserProfile::slotFillDataToForm() {
  _userData._login = QString::fromStdString(
    client_session_ptr_->getActiveUserCl()->getLogin());
  _userData._name = QString::fromStdString(
    client_session_ptr_->getActiveUserCl()->getUserName());
  _userData._email = QString::fromStdString(
    client_session_ptr_->getActiveUserCl()->getEmail());
  _userData._phone = QString::fromStdString(
    client_session_ptr_->getActiveUserCl()->getPhone());

  ui->loginLineEdit->setText(_userData._login);
  ui->nameLineEdit->setText(_userData._name);
  ui->emailLineEdit->setText(_userData._email);
  ui->phoneLineEdit->setText(_userData._phone);
}

void ScreenUserProfile::slotClearDataOnForm() {
  _userData._name = "";
  _userData._email = "";
  _userData._phone = "";

  _isNameChanged = false;
  _isEmailChanged = false;
  _isPhoneChanged = false;
  _isPasswordChanged = false;

  ui->passwordLineEdit->clear();
  ui->confirnPasswordLineEdit->clear();

  ui->savePushButton->setEnabled(false);
  ui->changePasswordPushButton->setEnabled(false);
}

void ScreenUserProfile::on_cancelPushButton_clicked() {
  slotClearDataOnForm();

  emit signalCloseUserProfile();
}

void ScreenUserProfile::on_savePushButton_clicked() {
  if (!_isNameChanged && !_isEmailChanged && !_isPhoneChanged)
    return;

  UserDTO userDTO;
  userDTO.login = ui->loginLineEdit->text().toStdString();
  userDTO.userName = ui->nameLineEdit->text().toStdString();
  userDTO.email = ui->emailLineEdit->text().toStdString();
  userDTO.phone = ui->phoneLineEdit->text().toStdString();
  userDTO.ban_until = 0;
  userDTO.disable_reason = "";
  userDTO.disabled_at = 0;
  userDTO.is_active = true;

  _isNameChanged = false;
  _isEmailChanged = false;
  _isPhoneChanged = false;
  _isPasswordChanged = false;

  if (client_session_ptr_->changeUserDataCl(userDTO))
    QMessageBox::information(this, "Message", "Data changed.");
  else
    QMessageBox::critical(this, "Error", "Data not changed.");

  ui->savePushButton->setEnabled(true);
}

void ScreenUserProfile::on_nameLineEdit_editingFinished() {
  if (ui->nameLineEdit->text() == _userData._name) {
    _isNameChanged = false;
    return;
  }

  if (ui->nameLineEdit->text().size() < 2 ||
      ui->nameLineEdit->text().size() > 20) {
    QMessageBox::critical(
      this, "Error",
      "Invalid name length. Length must be between 2 and 20 characters.");
    ui->nameLineEdit->setStyleSheet("color: red;");
    _isNameChanged = false;
    ui->savePushButton->setEnabled(false);
    return;
  }

  if (!engAndFiguresCheck(ui->nameLineEdit->text().toStdString())) {
    QMessageBox::critical(
      this, "Error",
      "Invalid name. Hover over the field for a hint.");
    ui->nameLineEdit->setStyleSheet("color: red;");
    _isNameChanged = false;
    ui->savePushButton->setEnabled(false);
  } else {
    ui->nameLineEdit->setStyleSheet("color: black;");
    ;
    ui->savePushButton->setEnabled(true);
    _isNameChanged = true;
  }
}

void ScreenUserProfile::on_emailLineEdit_editingFinished() {
  if (ui->emailLineEdit->text() == _userData._email) {
    _isEmailChanged = false;
    return;
  } else {
    _isEmailChanged = true;
    ui->savePushButton->setEnabled(true);
  }
}

void ScreenUserProfile::on_phoneLineEdit_editingFinished() {
  if (ui->phoneLineEdit->text() == _userData._phone) {
    _isPhoneChanged = false;
    return;
  } else {
    _isPhoneChanged = true;
    ui->savePushButton->setEnabled(true);
  }
}

void ScreenUserProfile::on_passwordLineEdit_editingFinished() {
  if (ui->passwordLineEdit->text().isEmpty()) {
    ui->passwordLineEdit->text().clear();
    ui->passwordLineEdit->setStyleSheet("color: black;");
    _isPasswordChanged = false;
    ui->changePasswordPushButton->setEnabled(false);
  }

  if (ui->passwordLineEdit->text().size() < 5 ||
      ui->passwordLineEdit->text().size() > 20) {
    QMessageBox::critical(
      this, "Error",
      "Invalid password length. Length must be between 5 and 20 characters.");
    ui->passwordLineEdit->setStyleSheet("color: red;");
    _isPasswordChanged = false;
    ui->changePasswordPushButton->setEnabled(false);
    return;
  }

  if (!checkNewLoginPasswordForLimits(
        ui->passwordLineEdit->text().toStdString(), 5, 20, true)) {
    QMessageBox::critical(
      this, "Error",
      "Invalid password. Hover over the field for a hint.");
    ui->passwordLineEdit->setStyleSheet("color: red;");
    _isPasswordChanged = false;
  }

  ui->passwordLineEdit->setStyleSheet("color: black;");
  ;

  if (!ui->confirnPasswordLineEdit->text().isEmpty() &&
      ui->confirnPasswordLineEdit->text() == ui->passwordLineEdit->text()) {
    ui->passwordLineEdit->setStyleSheet("color: black;");
    ;
    _isPasswordChanged = true;
    ui->changePasswordPushButton->setEnabled(true);
  } else {
    if (!ui->confirnPasswordLineEdit->text().isEmpty() &&
        ui->confirnPasswordLineEdit->text() != ui->passwordLineEdit->text())
      ui->passwordLineEdit->setStyleSheet("color: red;");
    else
      ui->confirnPasswordLineEdit->setStyleSheet("");

    _isPasswordChanged = false;
    ui->changePasswordPushButton->setEnabled(false);
  }
}

void ScreenUserProfile::on_confirnPasswordLineEdit_editingFinished() {
  if (ui->confirnPasswordLineEdit->text() != ui->passwordLineEdit->text()) {
    QMessageBox::critical(this, "Error", "Passwords do not match.");
    ui->confirnPasswordLineEdit->setStyleSheet("color: red;");
    ui->changePasswordPushButton->setEnabled(false);
    _isPasswordChanged = false;
  } else {
    ui->confirnPasswordLineEdit->setStyleSheet("color: black;");
    ;
    ui->changePasswordPushButton->setEnabled(true);
    _isPasswordChanged = true;
  }
}

void ScreenUserProfile::on_changePasswordPushButton_clicked() {
  UserDTO userDTO;
  userDTO.login = ui->loginLineEdit->text().toStdString();
  userDTO.passwordhash =
    picosha2::hash256_hex_string(ui->passwordLineEdit->text().toStdString());

  if (client_session_ptr_->changeUserPasswordCl(userDTO))
    QMessageBox::information(this, "Message", "Password changed.");
  else
    QMessageBox::critical(this, "Error", "Password not changed.");
}

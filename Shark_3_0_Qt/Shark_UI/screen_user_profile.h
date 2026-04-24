#ifndef SCREEN_USER_PROFILE_H
#define SCREEN_USER_PROFILE_H

#include <QString>
#include <QWidget>

#include "client_session.h"

namespace Ui {
class ScreenUserProfile;
}

struct UserDataQt {
  QString _login;
  QString _name;
  QString _email;
  QString _phone;
};

class ScreenUserProfile : public QWidget {
  Q_OBJECT

public:
  explicit ScreenUserProfile(QWidget *parent = nullptr);
  ~ScreenUserProfile();
  void setDatabase(std::shared_ptr<ClientSession> client_session_ptr);

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

signals:
  void signalCloseUserProfile();

public slots:
  void slotFillDataToForm();
  void slotClearDataOnForm();

private slots:
  void on_cancelPushButton_clicked();
  void on_savePushButton_clicked();
  void on_nameLineEdit_editingFinished();
  void on_emailLineEdit_editingFinished();
  void on_phoneLineEdit_editingFinished();
  void on_passwordLineEdit_editingFinished();
  void on_confirnPasswordLineEdit_editingFinished();
  void on_changePasswordPushButton_clicked();

private:
  Ui::ScreenUserProfile *ui;
  bool _isNameChanged{false};
  bool _isEmailChanged{false};
  bool _isPhoneChanged{false};
  bool _isPasswordChanged{false};
  UserDataQt _userData;
  std::shared_ptr<ClientSession> client_session_ptr_;
};

#endif // SCREEN_USER_PROFILE_H

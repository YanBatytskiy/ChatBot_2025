#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "client_session.h"
#include "logger.h"
#include <QMainWindow>
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(std::shared_ptr<ClientSession> client_session_ptr,

             std::shared_ptr<Logger> logger_ptr, QWidget *parent = nullptr);
  ~MainWindow();

  void setLoginForm();
  void setRegistrationForm();
  void setworkForm();

signals:
  void signalChangeUserData();

public slots:
  void onLoggedIn(QString login);
  void slotOnLogOut();
  void slotonRejectedRequested();
  void slotShowProfile();
  void slotCloseProfile();

private slots:

private:
  Ui::MainWindow *ui;
  std::shared_ptr<ClientSession> client_session_ptr_;
  std::shared_ptr<Logger> logger_ptr_;
};
#endif // MAINWINDOW_H

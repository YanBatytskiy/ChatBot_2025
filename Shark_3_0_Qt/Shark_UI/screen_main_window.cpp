#include "screen_main_window.h"

#include <QPushButton>
#include <QTimeZone>

#include "logger.h"
#include "screen_login.h"
#include "ui_screen_main_window.h"

MainWindow::MainWindow(std::shared_ptr<ClientSession> client_session_ptr,
                       std::shared_ptr<Logger> logger_ptr, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  client_session_ptr_ = std::move(client_session_ptr);
  logger_ptr_ = std::move(logger_ptr);

  ui->setupUi(this);

  ui->pageLogin->setDatabase(client_session_ptr_, logger_ptr_);
  ui->pageRegister->setDatabase(client_session_ptr_, logger_ptr_);
  ui->pageWork->setDatabase(client_session_ptr_, logger_ptr_);

  connect(ui->pageLogin, &ScreenLogin::signal_registration_requested, this,
          &MainWindow::setRegistrationForm);

  connect(ui->pageLogin, &ScreenLogin::rejected, this,
          &MainWindow::slotonRejectedRequested);

  connect(ui->pageLogin, &ScreenLogin::accepted, this, &MainWindow::onLoggedIn);

  connect(ui->pageRegister, &ScreenRegister::loginRequested, this,
          &MainWindow::setLoginForm);

  connect(ui->pageRegister, &ScreenRegister::signalLoggedIn, this,
          &MainWindow::onLoggedIn);

  connect(ui->pageRegister, &ScreenRegister::rejected, this,
          &MainWindow::slotonRejectedRequested);

  if (auto w = ui->pageWork) {
    connect(ui->pageWork, &ScreenMainWork::signalLogOut, this,
            &MainWindow::slotOnLogOut);

    connect(ui->pageWork, &ScreenMainWork::signalShowProfile, this,
            &MainWindow::slotShowProfile);
  }

  auto profile = ui->mainWindowstackedWidget->findChild<ScreenUserProfile *>(
      "pageProfile");

  connect(profile, &ScreenUserProfile::signalCloseUserProfile, this,
          &MainWindow::setworkForm);

  profile->setDatabase(client_session_ptr_);

  setLoginForm();
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::setLoginForm() {
  ui->pageLogin->clearFields();
  QWidget *page =
      ui->mainWindowstackedWidget->findChild<QWidget *>("pageLogin");

  ui->mainWindowstackedWidget->setCurrentWidget(page);
}

void MainWindow::setRegistrationForm() {
  ui->pageRegister->clearFields();
  QWidget *page =
      ui->mainWindowstackedWidget->findChild<QWidget *>("pageRegister");

  ui->mainWindowstackedWidget->setCurrentWidget(page);
}

void MainWindow::setworkForm() {
  QWidget *page = ui->mainWindowstackedWidget->findChild<QWidget *>("pageWork");

  ui->mainWindowstackedWidget->setCurrentWidget(page);
}

void MainWindow::onLoggedIn(QString login) {

  client_session_ptr_->clearChatSystemCl();

  client_session_ptr_->registerClientToSystemCl(login.toStdString());
  ui->pageWork->createSession();
  setworkForm();
}

void MainWindow::slotOnLogOut() {
  client_session_ptr_->clearChatSystemCl();
  setLoginForm();
}

void MainWindow::slotonRejectedRequested() {
  if (client_session_ptr_) {
    client_session_ptr_
        ->stopConnectionThreadCl(); // ← остановка фонового соединения
  }
  close();
}

void MainWindow::slotShowProfile() {
  QWidget *page =
      ui->mainWindowstackedWidget->findChild<QWidget *>("pageProfile");
  ui->mainWindowstackedWidget->setCurrentWidget(page);

  auto w = ui->mainWindowstackedWidget->findChild<ScreenUserProfile *>(
      "pageProfile");
  auto b = w->findChild<QPushButton *>("savePushButton");
  b->setEnabled(false);

  b = w->findChild<QPushButton *>("changePasswordPushButton");
  b->setEnabled(false);

  w->slotFillDataToForm();
}

void MainWindow::slotCloseProfile() {
  QWidget *page = ui->mainWindowstackedWidget->findChild<QWidget *>("pageWork");
  ui->mainWindowstackedWidget->setCurrentWidget(page);
}

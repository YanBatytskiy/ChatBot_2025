#ifndef SCREEN_LOGGER_FORM_H
#define SCREEN_LOGGER_FORM_H

#include <QStandardItemModel>
#include <QWidget>

#include "client_session.h"
#include "logger.h"

namespace Ui {
class ScreenLoggerForm;
}

class ScreenLoggerForm : public QWidget {
  Q_OBJECT

public:
  explicit ScreenLoggerForm(std::shared_ptr<ClientSession> client_session_ptr, std::shared_ptr<Logger> logger_ptr, QWidget *parent = nullptr);
  ~ScreenLoggerForm();

public slots:
  void slot_fill_logger_model(const std::multimap<qint64, QString> &logger_data);

private slots:
  void on_ExitPushButton_clicked();

private:
  Ui::ScreenLoggerForm *ui;
  QStandardItemModel *model_logger_{nullptr};
  std::shared_ptr<Logger> logger_ptr_;
  std::shared_ptr<ClientSession> client_session_ptr_;
};

#endif // SCREEN_LOGGER_FORM_H

#include "screen_logger_form.h"
#include "ui_screen_logger_form.h"
#include <QStandardItemModel>

ScreenLoggerForm::ScreenLoggerForm(std::shared_ptr<ClientSession> client_session_ptr,
                                   std::shared_ptr<Logger> logger_ptr, QWidget *parent) : QWidget(parent),
                                                                                         ui(new Ui::ScreenLoggerForm) {
  ui->setupUi(this);

  client_session_ptr_ = client_session_ptr;
  logger_ptr_ = logger_ptr;

  model_logger_ = new QStandardItemModel(0, 2);
}

ScreenLoggerForm::~ScreenLoggerForm() {
  delete ui;
}

void ScreenLoggerForm::slot_fill_logger_model(const std::multimap<qint64, QString> &logger_data) {

  model_logger_->clear();
  model_logger_->setHorizontalHeaderLabels({"#", "Text"});

  for (const auto &line : logger_data) {
    model_logger_->appendRow({new QStandardItem(QString::number(line.first)),
                              new QStandardItem(line.second)});
  } // for

  // Установить выравнивание для КАЖДОЙ строки во второй колонке
  for (int row = 0; row < model_logger_->rowCount(); ++row) {
    model_logger_->setData(model_logger_->index(row, 1), Qt::TextWordWrap, Qt::TextAlignmentRole);
  }

  ui->logTableView->setModel(model_logger_);
  ui->logTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->logTableView->verticalHeader()->setVisible(false);

  // Жирный шрифт
  ui->logTableView->horizontalHeader()->setFont(QFont("Arial", 12, QFont::Bold));

  // Серый фон
  ui->logTableView->horizontalHeader()->setStyleSheet(
      "QHeaderView::section { background-color: gray; color: white; }");

  ui->logTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->logTableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  ui->logTableView->setColumnWidth(0, ui->logTableView->columnWidth(0));
  ui->logTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
}

// void ScreenLoggerForm::ShowEvent(QShowEvent *event) {
//   QWidget::showEvent(event);

//   slot

// }

void ScreenLoggerForm::on_ExitPushButton_clicked() {
  close();
}

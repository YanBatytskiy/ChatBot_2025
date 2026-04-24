// #include "message/message_content.h"
// #include "message/message_content_struct.h"

// #include "models/model_chat_list.h"
#include "models/model_chat_mess_delegate.h"
#include "models/model_chat_messages.h"
// #include "system/date_time_utils.h"

// #include "message_text_browser.h"

#include "screen_chatting.h"
#include "ui_screen_chatting.h"

ScreenChatting::ScreenChatting(QWidget *parent)
    : QWidget(parent), ui(new Ui::ScreenChatting) {
  ui->setupUi(this);

  // Assigns the list item rendering delegate.
  ui->ScreenChattingMessagesList->setItemDelegate(
    new model_chat_mess_delegate(ui->ScreenChattingMessagesList));

  // Disables "uniform item height for all rows"
  ui->ScreenChattingMessagesList->setUniformItemSizes(false); // height is set by the delegate

  // Removes the default spacing between rows
  ui->ScreenChattingMessagesList->setSpacing(0); // we draw the separator ourselves

  // Makes scrolling work pixel-by-pixel rather than row-by-row.
  ui->ScreenChattingMessagesList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

  // Makes scrolling work pixel-by-pixel rather than row-by-row.
  ui->ScreenChattingMessagesList->setVerticalScrollMode(
    QAbstractItemView::ScrollPerPixel);
}

ScreenChatting::~ScreenChatting() {
  delete ui;
}

void ScreenChatting::setDatabase(std::shared_ptr<ClientSession> client_session_ptr) {
  client_session_ptr_ = client_session_ptr;
}

void ScreenChatting::setModel(MessageModel *messageModel) {
  ui->ScreenChattingMessagesList->setModel(messageModel);
  _messageModel = messageModel;
}

QTextEdit *ScreenChatting::getScreenChattingNewMessageTextEdit() const {
  return ui->ScreenChattingNewMessageTextEdit;
}

void ScreenChatting::on_ScreenChattingSendMessagePushButton_clicked() {
  if (ui->ScreenChattingNewMessageTextEdit->toPlainText() != "") {

    emit signalSendMessage();
  }
}

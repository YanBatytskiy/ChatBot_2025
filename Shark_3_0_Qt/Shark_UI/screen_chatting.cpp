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


  // Назначает делегат отрисовки элементов списка.
  ui->ScreenChattingMessagesList->setItemDelegate(
      new model_chat_mess_delegate(ui->ScreenChattingMessagesList));

 // Отключает “одинаковую высоту для всех строк
  ui->ScreenChattingMessagesList->setUniformItemSizes(false); // высоту задаёт делегат

 // Убирает стандартный промежуток между строками
  ui->ScreenChattingMessagesList->setSpacing(0); // разделитель рисуем сами

 // Заставляет прокрутку работать по пикселям, а не по строкам.
  ui->ScreenChattingMessagesList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

  // Заставляет прокрутку работать по пикселям, а не по строкам.
  ui->ScreenChattingMessagesList->setVerticalScrollMode(
      QAbstractItemView::ScrollPerPixel);

}

ScreenChatting::~ScreenChatting() { delete ui; }

void ScreenChatting::setDatabase(std::shared_ptr<ClientSession> client_session_ptr) {
  client_session_ptr_ = client_session_ptr;
}

void ScreenChatting::setModel(MessageModel *messageModel) {
  ui->ScreenChattingMessagesList->setModel(messageModel);
  _messageModel = messageModel;
}

QTextEdit *ScreenChatting::getScreenChattingNewMessageTextEdit() const
{
return ui->ScreenChattingNewMessageTextEdit;
}


void ScreenChatting::on_ScreenChattingSendMessagePushButton_clicked()
{
  if (ui->ScreenChattingNewMessageTextEdit->toPlainText() != "") {

    emit signalSendMessage();

  }
}


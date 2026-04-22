#ifndef CHATMESSAGESSCREEN_H
#define CHATMESSAGESSCREEN_H

#include "client_session.h"
#include "models/model_chat_messages.h"
#include <QAbstractItemView>
#include <QWidget>
#include <QTextEdit>

namespace Ui {
class ScreenChatting;
}

class ScreenChatting : public QWidget {
  Q_OBJECT

public:
  explicit ScreenChatting(QWidget *parent = nullptr);
  ~ScreenChatting();

  void setDatabase(std::shared_ptr<ClientSession> client_session_ptr);
  void setModel(MessageModel *messageModel);

  QTextEdit* getScreenChattingNewMessageTextEdit() const;

signals:
  void signalSendMessage();

private slots:    
    void on_ScreenChattingSendMessagePushButton_clicked();

private:
  Ui::ScreenChatting *ui;
  std::shared_ptr<ClientSession> client_session_ptr_;
  MessageModel *_messageModel{nullptr};

};

#endif // CHATMESSAGESSCREEN_H

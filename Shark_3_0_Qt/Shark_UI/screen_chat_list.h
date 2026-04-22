#ifndef CHAT_LIST_VIEW_SCREEN_H
#define CHAT_LIST_VIEW_SCREEN_H

#include "client_session.h"
#include <QWidget>
#include "model_chat_list.h"
#include <QAbstractItemView>

namespace Ui {
class ScreenChatList;
}

class ScreenChatList : public QWidget {
  Q_OBJECT
public:
  explicit ScreenChatList(QWidget *parent = nullptr);
  ~ScreenChatList();
  void setDatabase(std::shared_ptr<ClientSession> client_session_ptr);
  void setModel(ChatListModel *chatListModel);
  QItemSelectionModel* getSelectionModel() const;

  QModelIndex getCcurrentChatIndex() const;

public slots:
  void slotOnUserListIndexChanged(const QModelIndex &current,
                            const QModelIndex &previous);


signals:
  void signalCurrentChatIndexChanged(const QModelIndex &current,
                               const QModelIndex &previous);

  void signalUserListIdChanged(std::string login);

private:
  Ui::ScreenChatList *ui;
  std::shared_ptr<ClientSession> client_session_ptr_;
};

#endif // CHAT_LIST_VIEW_SCREEN_H

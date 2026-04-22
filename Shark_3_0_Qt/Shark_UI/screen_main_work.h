#ifndef WORKWINDOW_H
#define WORKWINDOW_H

#include "client_session.h"

#include "logger.h"
#include "models/model_chat_list.h"
#include "models/model_chat_messages.h"
#include "models/model_user_list.h"

#include <QButtonGroup>
#include <QMessageBox>
#include <QStringListModel>
#include <QWidget>
#include <memory>

namespace Ui {
class ScreenMainWork;
}

class ScreenMainWork : public QWidget {
  Q_OBJECT

public:
  explicit ScreenMainWork(QWidget *parent = nullptr);
  ~ScreenMainWork();
  void setDatabase(std::shared_ptr<ClientSession> client_session_ptr, std::shared_ptr<Logger> logger_ptr);

  void fillOneChatListModelWithData(const std::pair<std::size_t, ChatDTO> chat, bool newChatBool);
  void fillChatListModelWithData(bool allChats);

  void fillUserListModelWithData();
  void fillMessageModelWithData(std::size_t chatId);
  void clearMessageModelWithData();

  void ResetViewToInit();

  void createSession();
  void setupUserList();
  void setupScreenChatting();

  void clearChatListModelWithData();
  void refillChatListModelWithData(bool allChats);

  void sendMessageCommmand(const QModelIndex idx,
                           const std::size_t currentChatId,
                           const QString &newMessageText, bool newChatBool);
  void resetCountUnreadMessagesCommmand();

signals:
  void signalStartNewChat(bool turnOn);
  void signalAddContactToNewChat(UserListModel* newChatUserListModel, const QString &value);
  void signalClearUserDataToLabels();
  void signalMainWorkTransferrNewChatToMainChatList();
  void signalLogOut();
  void signalShowProfile();

public slots:

  void slotConnectUserListToChatListchange(bool status);

  void onConnectionStatusChanged(bool connectionStatus,
                                 ServerConnectionMode mode);
  void slotAddUserToNewChatList();
  void slotCancelNewChat();
  void slotMakeNewChat(int quantity, const QStringListModel* participantsListModel);
  void slotNewChatUserListBecameEnabled();

  void slotMainWorkTransferrNewChatToMainChatList();

  void slotFindContactsByPart();

private slots:
  void on_mainWorkChatUserTabWidget_currentChanged(int index);

  void on_createNewChatPushButton_clicked();

  void on_addUserToChatPushButton_clicked();

  void on_findLineEdit_textChanged(const QString &arg1);

  void on_findLineEdit_editingFinished();

  void on_mainWorkUsersList_doubleClicked(const QModelIndex &index);

  void on_logOutPushButton_clicked();

  void on_ProfilePushButton_clicked();

private:
  Ui::ScreenMainWork *ui;
  std::shared_ptr<ClientSession> client_session_ptr_;
  std::shared_ptr<Logger> logger_ptr_;

  QButtonGroup *_findRadoButtonGroup{nullptr};
  ChatListModel *_ChatListModel{nullptr};
  UserListModel *_userListModel{nullptr};
  MessageModel *_MessageModel{nullptr};
  UserListModel *_newChatUserListModel{nullptr};
  ChatDTO _newChatDTO;

  bool _startFind{true};
  bool _newChatMode{false};

  QMetaObject::Connection _connUserListToChatListChange;
};

#endif // WORKWINDOW_H

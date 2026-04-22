#ifndef SCREEN_NEW_CHAT_PARTICIPANTS_H
#define SCREEN_NEW_CHAT_PARTICIPANTS_H

#include <QListView>
#include <QStringListModel>
#include "models/model_user_list.h"

#include <QWidget>

namespace Ui {
class ScreenNewChatParticipants;
}

class ScreenNewChatParticipants : public QWidget {
  Q_OBJECT

public:
  explicit ScreenNewChatParticipants(QWidget *parent = nullptr);
  ~ScreenNewChatParticipants();

signals:
  void signalCancelNewChat();
  void signalMakeNewChat(int quantity, const QStringListModel* participantsListModel);
  void signalNewChatUserListBecameEnabled();


public slots:
  void slotCollectParticipantsForNewChat(bool turnOn);
  void slotAddContactToParticipantsList(UserListModel* newChatUserListModel, const QString &value);
  void slotScreenNewChatTransferrNewChatToMainChatList();

private slots:

  void on_screenNewChatDeleteContactPushButton_clicked();

  void on_screenUserDataCancelNewChatPushButton_clicked();

  void on_screenUserDataCreateNewChatPushButton_clicked();

private:
  Ui::ScreenNewChatParticipants *ui;
  QStringListModel *_participantsListModel{nullptr};
};

#endif // SCREEN_NEW_CHAT_PARTICIPANTS_H

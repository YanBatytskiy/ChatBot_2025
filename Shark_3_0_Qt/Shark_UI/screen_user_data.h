#ifndef SCREEN_USER_DATA_H
#define SCREEN_USER_DATA_H

#include <QWidget>
#include <QPushButton>
#include <QModelIndex>
#include "client_session.h"
#include "model_user_list.h"

namespace Ui {
class ScreenUserData;
}

class ScreenUserData : public QWidget {
  Q_OBJECT

public:
  explicit ScreenUserData(QWidget *parent = nullptr);
  ~ScreenUserData();
  QPushButton* blockPushButton;
  QPushButton* unblockPushButton;
  QPushButton* bunToPushButton;
  QPushButton* unBunToPushButton;
  void setDatabase(std::shared_ptr<ClientSession> client_session_ptr,
                   UserListModel* user_list_model);

  void slotClearUserDataToLabels();

  void setUserDataToLabels(const QModelIndex& index);

signals:
  void signalNewChatListBecameEnabled();
  void signal_on_block_push_button_clicked();
  void signal_on_unblock_push_button_clicked();
  void signal_on_bun_push_button_clicked();
  void signal_on_unbun_push_button_clicked();

private slots:
  void slot_on_block_push_button_clicked();
  void slot_on_unblock_push_button_clicked();
  void slot_on_bun_push_button_clicked();
  void slot_on_unbun_push_button_clicked();

private:
  Ui::ScreenUserData *ui;
  std::shared_ptr<ClientSession> client_session_ptr_;
  UserListModel *_userListModel{nullptr};
  std::shared_ptr<QModelIndex> user_list_index_;

};

#endif // SCREEN_USER_DATA_H

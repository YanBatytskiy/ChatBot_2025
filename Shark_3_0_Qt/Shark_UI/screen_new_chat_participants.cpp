#include "screen_new_chat_participants.h"
#include "ui_screen_new_chat_participants.h"
#include "models/model_user_list.h"


#include <QObject>

ScreenNewChatParticipants::ScreenNewChatParticipants(QWidget *parent) : QWidget(parent),
                                                                        ui(new Ui::ScreenNewChatParticipants) {
  ui->setupUi(this);
  this->setEnabled(false);

  _participantsListModel = new QStringListModel(this);

  ui->screenUserDataNewChatUsersList->setModel(_participantsListModel);

}

ScreenNewChatParticipants::~ScreenNewChatParticipants() {
  delete ui;
}

void ScreenNewChatParticipants::slotCollectParticipantsForNewChat(bool turnOn) {

  if (turnOn) {
    this->setEnabled(true);
    emit signalNewChatUserListBecameEnabled();
  }
  else
    this->setEnabled(false);
}

void ScreenNewChatParticipants::slotAddContactToParticipantsList(UserListModel* newChatUserListModel,
                                                                 const QString &value) {

  const auto quantity = _participantsListModel->rowCount();

  for (const auto &line : _participantsListModel->stringList()) {
    if (line == value){
      newChatUserListModel->removeItem(newChatUserListModel->rowCount()-1);
      return;
    }
  }

  _participantsListModel->insertRow(quantity);

  const auto &idx = _participantsListModel->index(_participantsListModel->rowCount() - 1);

  _participantsListModel->setData(idx, value);
}

void ScreenNewChatParticipants::slotScreenNewChatTransferrNewChatToMainChatList() {

  this->setEnabled(false);

  ui->screenUserDataCreateNewChatPushButton->setEnabled(true);

  ui->screenNewChatDeleteContactPushButton->setEnabled(true);

  ui->screenUserDataNewChatUsersList->setEnabled(true);

  _participantsListModel->setStringList({});

  emit signalCancelNewChat();
}

void ScreenNewChatParticipants::on_screenNewChatDeleteContactPushButton_clicked() {
  const auto &selectModel = ui->screenUserDataNewChatUsersList->selectionModel();

  const auto &idx = selectModel->currentIndex();

  if (idx.isValid()) {
    _participantsListModel->removeRow(idx.row());
  }
}

void ScreenNewChatParticipants::on_screenUserDataCancelNewChatPushButton_clicked() {
  this->setEnabled(false);

  ui->screenUserDataCreateNewChatPushButton->setEnabled(true);

  ui->screenNewChatDeleteContactPushButton->setEnabled(true);

  ui->screenUserDataNewChatUsersList->setEnabled(true);

  _participantsListModel->setStringList({});
  emit signalCancelNewChat();
}


void ScreenNewChatParticipants::on_screenUserDataCreateNewChatPushButton_clicked()
{
  const auto quantity = _participantsListModel->rowCount();

  if (quantity > 0) {
    ui->screenUserDataCreateNewChatPushButton->setEnabled(false);
    ui->screenNewChatDeleteContactPushButton->setEnabled(false);
    ui->screenUserDataNewChatUsersList->setEnabled(false);
    emit signalMakeNewChat(quantity, _participantsListModel);
  }
}


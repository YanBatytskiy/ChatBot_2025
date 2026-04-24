#include "models/model_chat_list_delegate.h"
#include "model_user_list.h"
#include "screen_chat_list.h"
#include "ui_screen_chat_list.h"

#include <QItemSelectionModel>
#include <QObject>

ScreenChatList::ScreenChatList(QWidget *parent)
    : QWidget(parent), ui(new Ui::ScreenChatList) {
  ui->setupUi(this);
  // Assigns the list item rendering delegate.
  ui->chatListView->setItemDelegate(new ChatListItemDelegate(ui->chatListView));

  // Disables "uniform item height for all rows"
  ui->chatListView->setUniformItemSizes(false); // height is set by the delegate

  // Removes the default spacing between rows
  ui->chatListView->setSpacing(0); // we draw the separator ourselves

  // Gives the widget mouse move events even without pressed buttons.
  ui->chatListView->setMouseTracking(true);

  ui->chatListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  ui->chatListView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  ui->chatListView->setWordWrap(false);
  ui->chatListView->setTextElideMode(Qt::ElideNone);

  // ui->chatListView->setStyleSheet("QListView { background-color: #FFFFF0; }");
  ui->chatListView->setStyleSheet("background-color: #FFFFF0;");
}

ScreenChatList::~ScreenChatList() {
  delete ui;
}

void ScreenChatList::setDatabase(std::shared_ptr<ClientSession> client_session_ptr) {
  client_session_ptr_ = client_session_ptr;
}

void ScreenChatList::setModel(ChatListModel *chatListModel) {
  ui->chatListView->setModel(chatListModel);

  auto selectMode = ui->chatListView->selectionModel();

  QObject::connect(selectMode, &QItemSelectionModel::currentChanged, this,
                   &ScreenChatList::signalCurrentChatIndexChanged);

  if (chatListModel && chatListModel->rowCount() > 0) {
    ui->chatListView->setCurrentIndex(chatListModel->index(0, 0));
  }
}

QItemSelectionModel *ScreenChatList::getSelectionModel() const {
  return ui->chatListView->selectionModel();
}

QModelIndex ScreenChatList::getCcurrentChatIndex() const {

  auto v = ui->chatListView;
  QModelIndex idx = v->currentIndex();

  if (!idx.isValid() && v->selectionModel())
    idx = v->selectionModel()->currentIndex();
  return idx;
}

void ScreenChatList::slotOnUserListIndexChanged(const QModelIndex &current, const QModelIndex &previous) {

  std::string login;

  if (!current.isValid()) {
    emit signalUserListIdChanged(std::string{});
    return;
  }

  Q_UNUSED(previous);

  login = current.data(UserListModel::LoginRole).toString().toStdString();

  emit signalUserListIdChanged(login);
}

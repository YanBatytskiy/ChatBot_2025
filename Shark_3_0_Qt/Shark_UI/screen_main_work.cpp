#include "screen_main_work.h"

#include <QItemSelectionModel>
#include <QMessageBox>
#include <QScopedValueRollback>
#include <QTimeZone>

#include "dto/dto_struct.h"
#include "message/message_content.h"
#include "message/message_content_struct.h"
#include "model_chat_list.h"
#include "model_chat_messages.h"
#include "model_user_list.h"
#include "model_user_list_delegate.h"
#include "screen_chat_list.h"
#include "screen_chatting.h"
#include "screen_new_chat_participants.h"
#include "system/date_time_utils.h"
#include "ui_screen_main_work.h"
#include "user/user.h"

ScreenMainWork::ScreenMainWork(QWidget *parent)
    : QWidget(parent), ui(new Ui::ScreenMainWork) {
  ui->setupUi(this);

  _newChatMode = false;

  if (auto w =
        ui->mainWorkPageUserDataView->findChild<ScreenNewChatParticipants *>(
          "ScreenNewChatParticipantsWidget")) {
    // connection: "start new chat" button in the contacts list -> slot
    // that enables the form and fills it
    connect(this, &ScreenMainWork::signalStartNewChat, w,
            &ScreenNewChatParticipants::slotCollectParticipantsForNewChat);

    // connection: "cancel new chat" button -> slot that disables the form and
    // re-enables the chat list and message editor
    connect(w, &ScreenNewChatParticipants::signalCancelNewChat, this,
            &ScreenMainWork::slotCancelNewChat);

    // connection: "send message to participants" button -> slot that switches to
    // the chat list form and message editor
    connect(w, &ScreenNewChatParticipants::signalMakeNewChat, this,
            &ScreenMainWork::slotMakeNewChat);

    // connection: after a message in a new chat is sent successfully -> slot that
    // switches all forms back to the main chat list mode
    connect(this, &ScreenMainWork::signalMainWorkTransferrNewChatToMainChatList,
            w,
            &ScreenNewChatParticipants::
              slotScreenNewChatTransferrNewChatToMainChatList);

    // connection: add contact signal -> slot that adds the contact to the list
    connect(this, &ScreenMainWork::signalAddContactToNewChat, w,
            &ScreenNewChatParticipants::slotAddContactToParticipantsList);

    // connection: enabling the new chat recipients list signal -> slot that
    // disables the block and ban buttons
    connect(w, &ScreenNewChatParticipants::signalNewChatUserListBecameEnabled,
            this, &ScreenMainWork::slotNewChatUserListBecameEnabled);
  }

  // connection: start editing contact search -> clear user data on the form
  connect(this, &ScreenMainWork::signalClearUserDataToLabels,
          ui->mainWorkPageUserDataView,
          &ScreenUserData::slotClearUserDataToLabels, Qt::UniqueConnection);
}

ScreenMainWork::~ScreenMainWork() {
  delete ui;
}

void ScreenMainWork::setDatabase(
  std::shared_ptr<ClientSession> client_session_ptr,
  std::shared_ptr<Logger> logger_ptr) {
  client_session_ptr_ = client_session_ptr;
  logger_ptr_ = logger_ptr;

  if (auto *chat_list = ui->mainWorkTabChatsList) {
    chat_list->setDatabase(client_session_ptr_);
  }

  if (auto *chatting = ui->mainWorkPageChatting) {
    chatting->setDatabase(client_session_ptr_);
  }

  if (auto *user_data = ui->mainWorkPageUserDataView) {
    user_data->setDatabase(client_session_ptr_, _userListModel);
  }

  ui->findLineEdit->setEnabled(false);
  ui->mainWorkChatUserTabWidget->setCurrentIndex(0);
  ui->mainWorkRightStackedWidget->setCurrentIndex(0);

  connect(client_session_ptr_.get(), &ClientSession::serverStatusChanged, this,
          &ScreenMainWork::onConnectionStatusChanged, Qt::QueuedConnection);
}

void ScreenMainWork::fillOneChatListModelWithData(
  const std::pair<std::size_t, ChatDTO> chat, bool newChatBool) {
  QString participantsChatList;
  QString infoText;

  int unreadCount;
  bool isMuted;
  std::int64_t lastTime;

  // add the chat number
  const auto chatIdStr = QString::number(chat.second.chatId);

  // add the unread count

  if (chat.second.chatId != 0) {
    unreadCount =
      client_session_ptr_->getInstanceCl()
        .getChatById(chat.second.chatId)
        ->getUnreadMessageCount(client_session_ptr_->getActiveUserCl());
  } else
    unreadCount = 0;

  // add the timestamp of the last message

  std::string dateTimeStamp;

  if (newChatBool) {
    lastTime = 0;
    dateTimeStamp = "---";
  } else {
    lastTime = static_cast<std::int64_t>(chat.first);
    dateTimeStamp = formatTimeStampToString(chat.first, true);
  }

  infoText = _ChatListModel->buildInfoTextForRow(
    chatIdStr, QString::number(unreadCount),
    QString::fromStdString(dateTimeStamp));

  // build the participants string with logins and names
  participantsChatList = "Name (Login: ";

  bool first = true;
  for (const auto &participant : chat.second.participants) {
    // retrieve the user login and name
    const auto &login = participant.login;

    // skip the active user
    if (login == client_session_ptr_->getActiveUserCl()->getLogin())
      continue;

    std::string userName;

    if (!newChatBool) {
      const auto &user =
        client_session_ptr_->getInstanceCl().findUserByLogin(login);
      userName = user->getUserName();
    } else
      userName =
        _newChatUserListModel->findNameByLogin(QString::fromStdString(login))
          .toStdString();

    if (!first)
      participantsChatList += ", ";

    participantsChatList += QString::fromStdString(userName) + " (" +
                            QString::fromStdString(login) + ")";
    first = false;
  } // for participants

  isMuted = false;

  _ChatListModel->fillChatListItem(participantsChatList, infoText, unreadCount,
                                   isMuted, lastTime, chat.second.chatId);
}

void ScreenMainWork::fillChatListModelWithData(bool allChats) {
  const auto listOfChat = client_session_ptr_->getChatListCl();

  if (listOfChat.has_value()) {
    std::string choosenContact;

    if (!allChats) {
      const QModelIndex &idx = ui->mainWorkUsersList->currentIndex();

      if (idx.isValid()) {
        choosenContact =
          idx.data(UserListModel::LoginRole).toString().toStdString();
      } else
        choosenContact.clear();
    }

    for (const auto &chat : listOfChat.value()) {
      if (!allChats) {
        bool ischoosenContact = false;
        for (const auto &participant : chat.second.participants) {
          // retrieve the login
          const auto &login = participant.login;

          // skip the active user
          if (login == client_session_ptr_->getActiveUserCl()->getLogin())
            continue;

          // check if the selected contact is in the chat participant list
          if (login == choosenContact) {
            ischoosenContact = true;
            break;
          }
        } // for

        // if this is the given user's chat, invoke chat population
        if (ischoosenContact)
          fillOneChatListModelWithData(chat, false);
      } // if
      else
        fillOneChatListModelWithData(chat, false);

    } // for listOfChat

  } // if has_value

  // Empty chat list is intentionally silent until a dedicated empty-state
  // message is added to the UI.
  else
    return;
}

void ScreenMainWork::fillUserListModelWithData() {
  if (auto sm = ui->mainWorkUsersList->selectionModel()) {
    QSignalBlocker b(sm);
    _userListModel->clear();
  }

  const auto users = client_session_ptr_->getInstanceCl().getUsers();

  if (users.size()) {
    for (const auto &user_ptr : users) {
      if (user_ptr != nullptr)
        if (user_ptr->getLogin() !=
            client_session_ptr_->getActiveUserCl()->getLogin()) {
          const QString login = QString::fromStdString(user_ptr->getLogin());
          const QString name = QString::fromStdString(user_ptr->getUserName());
          const QString email = QString::fromStdString(user_ptr->getEmail());
          const QString phone = QString::fromStdString(user_ptr->getPhone());
          const QString disableReason =
            QString::fromStdString(user_ptr->getUserName());
          bool isActive = user_ptr->getIsActive();
          const std::int64_t disableAt = user_ptr->getDisabledAt();
          const std::int64_t bunUntil = user_ptr->getBunUntil();

          _userListModel->fillUserItem(login, name, email, phone, disableReason,
                                       isActive, disableAt, bunUntil);
        } // if user_ptr
    } // for users

  } // if users
}

void ScreenMainWork::fillMessageModelWithData(std::size_t chatId) {
  _MessageModel->clear();

  if (chatId == 0)
    return; // new chat without an id
  const auto chat_ptr =
    client_session_ptr_->getInstanceCl().getChatById(chatId);
  if (!chat_ptr)
    return; // chat is not in the map

  const auto &messages = chat_ptr->getMessages();
  if (messages.empty())
    return;

  for (const auto &message : messages) {
    if (message.second != nullptr) {
      QString messageText = "";

      const auto &content = message.second->getContent();
      if (!content.empty()) {
        const auto &textContent =
          std::dynamic_pointer_cast<MessageContent<TextContent>>(content[0]);

        if (textContent)
          messageText =
            QString::fromStdString(textContent->getMessageContent()._text);
      }
      const auto sender = message.second->getSender();
      const auto sender_ptr = sender.lock();
      QString senderLogin = "";
      QString senderName = "";
      if (!sender_ptr) {
        senderLogin = "Deleted";
        senderName = "";
      } else {
        senderLogin = QString::fromStdString(sender_ptr->getLogin());
        senderName = QString::fromStdString(sender_ptr->getUserName());
      }
      std::int64_t timeStamp = static_cast<int64_t>(message.first);
      std::size_t messageId =
        static_cast<size_t>(message.second->getMessageId());

      _MessageModel->fillMessageItem(messageText, senderLogin, senderName,
                                     timeStamp, messageId);

    } // if message
  } // for messages
}

void ScreenMainWork::clearMessageModelWithData() {
  if (auto sm = ui->mainWorkTabChatsList->getSelectionModel()) {
    QSignalBlocker b1(sm);
    auto clInUser = ui->mainWorkPageUserDataView->findChild<ScreenChatList *>(
      "ScreenUserDataChatsListWidget");
    QSignalBlocker b2(clInUser ? clInUser->getSelectionModel() : nullptr);

    _MessageModel->clear();
  }
}

void ScreenMainWork::onConnectionStatusChanged(bool connectionStatus,
                                               ServerConnectionMode mode) {
  ServerConnectionConfig serverConnectionConfig;

  if (connectionStatus) {
    ui->serverStatusLabelRound->setStyleSheet(
      "background-color: green; border-radius: 8px;");
    ui->serverStatusLabel->setText("server online");

  } else {
    ui->serverStatusLabelRound->setStyleSheet(
      "background-color: red; border-radius: 8px;");
    ui->serverStatusLabel->setText("server offline");
  }
}

void ScreenMainWork::slotCancelNewChat() {
  // "contact chats" screen
  const auto chatListInUser =
    ui->mainWorkPageUserDataView->findChild<ScreenChatList *>(
      "ScreenUserDataChatsListWidget");
  if (!chatListInUser)
    return;

  // pointer to the object containing the messages window in single-contact
  // chat mode (from the address book)
  auto MessageChattingInUser =
    ui->mainWorkPageUserDataView->findChild<ScreenChatting *>(
      "ScreenUserDataMessagesListWidget");
  if (!MessageChattingInUser)
    return;

  if (auto panel = ui->mainWorkPageUserDataView->findChild<QWidget *>(
        "ScreenUserDataUserDataWidget")) {
    if (auto button1 = panel->findChild<QPushButton *>("banPushButton")) {
      button1->setEnabled(true);
    }
    if (auto button2 = panel->findChild<QPushButton *>("blockPushButton")) {
      button2->setEnabled(true);
    }
  }

  ui->createNewChatPushButton->setEnabled(true);
  ui->mainWorkUsersList->setEnabled(true);

  auto selectModel = ui->mainWorkUsersList->selectionModel();
  if (selectModel) {
    selectModel->setCurrentIndex(selectModel->model()->index(0, 0),
                                 QItemSelectionModel::ClearAndSelect);
  };

  auto ChatListInUser =
    ui->mainWorkPageUserDataView->findChild<ScreenChatList *>(
      "ScreenUserDataChatsListWidget");

  if (!ChatListInUser)
    return;

  selectModel = ChatListInUser->getSelectionModel();

  if (selectModel) {
    selectModel->setCurrentIndex(selectModel->model()->index(0, 0),
                                 QItemSelectionModel::ClearAndSelect);
  };

  ui->findLineEdit->setEnabled(true);
  ui->findLineEdit->setText({});

  ui->mainWorkChatUserTabWidget->setTabEnabled(0, true);

  _newChatMode = false;

  // connection: index change when a contact is selected -> populate the contact
  // chat list window
  slotConnectUserListToChatListchange(true);
}

void ScreenMainWork::slotMakeNewChat(
  int quantity, const QStringListModel *participantsListModel) {
  // "contact chats" screen
  const auto chatListInUser =
    ui->mainWorkPageUserDataView->findChild<ScreenChatList *>(
      "ScreenUserDataChatsListWidget");
  if (!chatListInUser)
    return;

  // pointer to the object containing the messages window in single-contact
  // chat mode (from the address book)
  auto MessageChattingInUser =
    ui->mainWorkPageUserDataView->findChild<ScreenChatting *>(
      "ScreenUserDataMessagesListWidget");
  if (!MessageChattingInUser)
    return;

  clearChatListModelWithData();
  clearMessageModelWithData();

  ui->findLineEdit->setEnabled(false);
  ui->mainWorkUsersList->setEnabled(false);

  // build the chat
  _newChatDTO.chatId = 0;
  _newChatDTO.senderLogin = client_session_ptr_->getActiveUserCl()->getLogin();
  _newChatDTO.participants.clear();

  ParticipantsDTO participantsDTO;

  // build the chat participants list
  for (int i = 0; i < quantity; ++i) {
    participantsDTO.login =
      participantsListModel->stringList().at(i).toStdString();
    participantsDTO.lastReadMessage = 0;
    participantsDTO.deletedMessageIds.clear();
    participantsDTO.deletedFromChat = false;

    _newChatDTO.participants.push_back(participantsDTO);
  }

  // populate the chat model with data
  auto chat = std::pair<std::size_t, ChatDTO>({0, _newChatDTO});

  clearChatListModelWithData();
  fillOneChatListModelWithData(chat, true);

  // enable the chat list on the right
  chatListInUser->setEnabled(true);

  // enable the messages window
  clearMessageModelWithData();

  MessageChattingInUser->setEnabled(true);

  ui->addUserToChatPushButton->setVisible(false);

  // block selection signals while setting the index
  if (auto sm = chatListInUser->getSelectionModel()) {
    QSignalBlocker blk(sm);
    sm->setCurrentIndex(sm->model()->index(0, 0),
                        QItemSelectionModel::ClearAndSelect);
  }
}

void ScreenMainWork::slotNewChatUserListBecameEnabled() {
  if (auto b = ui->mainWorkPageUserDataView->findChild<QPushButton *>(
        "blockPushButton"))
    b->setEnabled(false);
  if (auto b = ui->mainWorkPageUserDataView->findChild<QPushButton *>(
        "unblockPushButton"))
    b->setEnabled(false);
  if (auto b = ui->mainWorkPageUserDataView->findChild<QPushButton *>(
        "banPushButton"))
    b->setEnabled(false);
  if (auto b = ui->mainWorkPageUserDataView->findChild<QPushButton *>(
        "unBunPushButton"))
    b->setEnabled(false);
}

void ScreenMainWork::slotMainWorkTransferrNewChatToMainChatList() {
  // trigger processing in child windows
  emit signalMainWorkTransferrNewChatToMainChatList();
}

void ScreenMainWork::slotFindContactsByPart() {
  if (!ui->findLineEdit->isEnabled())
    return; // processing already in progress

  static bool busy = false;
  if (busy)
    return;

  QScopedValueRollback<bool> busyGuard(busy, true); // instead of manual resets

  const QSignalBlocker blockEdit(
    ui->findLineEdit); // 1) block signals immediately

  ui->findLineEdit->setEnabled(false); // 2) a single call is enough
  QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

  const auto textToFind = ui->findLineEdit->text().toStdString();

  if (textToFind.empty()) {
    _userListModel->clear();

    clearChatListModelWithData();
    clearMessageModelWithData();

    fillUserListModelWithData();
    refillChatListModelWithData(true);

    ui->findLineEdit->setEnabled(true);

    const auto selectModel = ui->mainWorkUsersList->selectionModel();

    selectModel->setCurrentIndex(selectModel->model()->index(0, 0),
                                 QItemSelectionModel::ClearAndSelect);

    ui->findLineEdit->setFocus();
    return;
  }

  emit signalClearUserDataToLabels();
  clearChatListModelWithData();
  clearMessageModelWithData();

  std::vector<UserDTO> userListDTO =
    client_session_ptr_->findUserByTextPartOnServerCl(textToFind);

  _userListModel->clear();

  if (!userListDTO.empty() && !userListDTO[0].login.empty()) {
    for (const auto &userDTO : userListDTO) {
      const QString login = QString::fromStdString(userDTO.login);
      const QString name = QString::fromStdString(userDTO.userName);
      const QString email = QString::fromStdString(userDTO.email);
      const QString phone = QString::fromStdString(userDTO.phone);
      const QString disableReason =
        QString::fromStdString(userDTO.disable_reason);
      bool isActive = userDTO.is_active;
      const std::int64_t disableAt = userDTO.disabled_at;
      const std::int64_t bunUntil = userDTO.ban_until;

      _userListModel->fillUserItem(login, name, email, phone, disableReason,
                                   isActive, disableAt, bunUntil);
    } // for userLisstDTO
  }
  ui->findLineEdit->setEnabled(true);
  ui->findLineEdit->setFocus();
}

void ScreenMainWork::ResetViewToInit() {
  // chats page

  // set the chats page
  ui->mainWorkChatUserTabWidget->setTabEnabled(0, true);
  ui->mainWorkChatUserTabWidget->setCurrentIndex(0);

  // set focus on the first element
  ui->mainWorkTabChatsList->getSelectionModel()->setCurrentIndex(
    ui->mainWorkTabChatsList->getSelectionModel()->model()->index(0, 0),
    QItemSelectionModel::ClearAndSelect);

  // disabled the search line
  ui->findLineEdit->setEnabled(false);
  ui->findLineEdit->setText("");

  // address book page

  // connected the contacts list
  ui->mainWorkUsersList->setEnabled(true);

  // connected the new chat button
  ui->createNewChatPushButton->setEnabled(true);

  // disabled the add-user button
  ui->addUserToChatPushButton->setVisible(false);

  // connection: index change when a contact is selected -> populate the contact
  // chat list window
  slotConnectUserListToChatListchange(true);

  emit signalStartNewChat(false);
}

void ScreenMainWork::createSession() {
  ui->serverStatusLabelRound->setStyleSheet(
    "background-color: green; border-radius: 8px;");
  ui->serverStatusLabel->setText("server online");

  ui->loginLabel->setText(
    " Login: " + QString::fromStdString(
                   client_session_ptr_->getActiveUserCl()->getLogin()));
  ui->nameLabel->setText(
    "Name: " + QString::fromStdString(
                 client_session_ptr_->getActiveUserCl()->getUserName()));

  // chat list working window
  _ChatListModel = new ChatListModel(this);
  ui->mainWorkTabChatsList->setModel(_ChatListModel);

  // connected the model to the second widget for displaying the chat list
  // of a specific contact
  ui->mainWorkPageUserDataView
    ->findChild<ScreenChatList *>("ScreenUserDataChatsListWidget")
    ->setModel(_ChatListModel);

  fillChatListModelWithData(true);

  // address book working window
  setupUserList();

  // handling chat messages
  setupScreenChatting();

  ResetViewToInit();

  ui->mainWorkTabChatsList->setFocus();
}

void ScreenMainWork::setupUserList() {
  // address book working window
  _userListModel = new UserListModel(ui->mainWorkUsersList);
  ui->mainWorkUsersList->setModel(_userListModel);

  fillUserListModelWithData();

  // Assigns the list item rendering delegate.
  ui->mainWorkUsersList->setItemDelegate(
    new UserListItemDelegate(ui->mainWorkUsersList));

  //        // Disables "uniform item height for all rows"
  // ui->mainWorkUsersList->setUniformItemSizes(false); // height is set by the delegate

  // Removes the default spacing between rows
  ui->mainWorkUsersList->setSpacing(0); // we draw the separator ourselves

  // Makes scrolling work pixel-by-pixel rather than row-by-row.
  ui->mainWorkUsersList->setVerticalScrollMode(
    QAbstractItemView::ScrollPerPixel);

  // Gives the widget mouse move events even without pressed buttons.
  ui->mainWorkUsersList->setMouseTracking(true);

  ui->mainWorkUsersList->setStyleSheet(
    "QListView { background-color: #FFFFF0; }");

  ui->addressBookLabel->setText("Contacts from the address book");

  // get the selection model of the second chat list (chat list of a specific
  // user)
  auto selectMode = ui->mainWorkUsersList->selectionModel();

  // connection: index change when a contact is selected -> populate the contact
  // data window
  QObject::connect(selectMode, &QItemSelectionModel::currentChanged,
                   ui->mainWorkPageUserDataView,
                   &ScreenUserData::setUserDataToLabels);

  // connection: index change when a contact is selected -> populate the contact
  // chat list window slotConnectUserListToChatListchange(true);

  if (auto *user_data = ui->mainWorkPageUserDataView) {
    user_data->setDatabase(client_session_ptr_, _userListModel);
  }
}

void ScreenMainWork::setupScreenChatting() {
  // handling chat messages

  // create the messages model
  _MessageModel = new MessageModel(ui->mainWorkPageChatting);

  // working with the window that shows all user chats

  // bound the model to the view on the all-chats page
  ui->mainWorkPageChatting->setModel(_MessageModel);

  // populated the model with a specific chat's data
  const auto &index = ui->mainWorkTabChatsList->getCcurrentChatIndex();

  if (index.isValid()) {
    const auto &currentChatId =
      static_cast<size_t>(index.data(ChatListModel::ChatIdRole).toLongLong());

    fillMessageModelWithData(currentChatId);
  }

  // connection: global chat list -> chat messages window
  // Refresh the message list when the active chat selection changes.
  connect(ui->mainWorkTabChatsList,
          &ScreenChatList::signalCurrentChatIndexChanged, this, [this](auto) {
            const auto &idx = ui->mainWorkTabChatsList->getCcurrentChatIndex();
            if (idx.isValid()) {
              auto currentChatId =
                idx.data(ChatListModel::ChatIdRole).toLongLong();
              fillMessageModelWithData(currentChatId);
            }
          });

  // reset the unread count for the first chat in the list
  resetCountUnreadMessagesCommmand();

  // Clear unread counters when the current chat changes.
  connect(ui->mainWorkTabChatsList,
          &ScreenChatList::signalCurrentChatIndexChanged, this,
          &ScreenMainWork::resetCountUnreadMessagesCommmand,
          Qt::UniqueConnection);

  // Forward the send-message signal from the chat widget to the session layer.
  connect(ui->mainWorkPageChatting, &ScreenChatting::signalSendMessage, this,
          [this] {
            QModelIndex idx;

            idx = ui->mainWorkTabChatsList->getCcurrentChatIndex();
            if (!idx.isValid())
              return;

            const auto &currentChatId = static_cast<size_t>(
              idx.data(ChatListModel::ChatIdRole).toLongLong());

            QString newMessageText;
            newMessageText =
              ui->mainWorkPageChatting->getScreenChattingNewMessageTextEdit()
                ->toPlainText();

            if (newMessageText.trimmed().isEmpty())
              return;

            sendMessageCommmand(idx, currentChatId, newMessageText, false);

            refillChatListModelWithData(true);

            const auto chatListInUser =
              ui->mainWorkPageUserDataView->findChild<ScreenChatList *>(
                "ScreenUserDataChatsListWidget");

            if (!chatListInUser)
              return;

            auto selectMode = ui->mainWorkTabChatsList->getSelectionModel();
            if (!selectMode)
              return;

            selectMode->setCurrentIndex(selectMode->model()->index(0, 0),
                                        QItemSelectionModel::ClearAndSelect);
          });

  // working with the specific contact chats window

  // "contact chats" screen
  const auto chatListInUser =
    ui->mainWorkPageUserDataView->findChild<ScreenChatList *>(
      "ScreenUserDataChatsListWidget");
  if (!chatListInUser)
    return;

  // pointer to the object containing the messages window in single-contact
  // chat mode (from the address book)
  auto MessageChattingInUser =
    ui->mainWorkPageUserDataView->findChild<ScreenChatting *>(
      "ScreenUserDataMessagesListWidget");
  if (!MessageChattingInUser)
    return;

  // bound the model to the second widget for displaying the chat list
  // of a specific contact
  MessageChattingInUser->setModel(_MessageModel);

  // get the SelectionModel from the specific user's chat list
  auto selectMode = chatListInUser->getSelectionModel();
  if (!selectMode)
    return;

  // connection: contact chat list -> chat messages window
  connect(chatListInUser, &ScreenChatList::signalCurrentChatIndexChanged, this,
          [this, chatListInUser](auto) {
            const auto &idx = chatListInUser->getCcurrentChatIndex();

            if (idx.isValid()) {
              auto currentChatId =
                idx.data(ChatListModel::ChatIdRole).toLongLong();
              fillMessageModelWithData(currentChatId);
            }
          });

  // connect to the clear-unread signal
  connect(chatListInUser, &ScreenChatList::signalCurrentChatIndexChanged, this,
          &ScreenMainWork::resetCountUnreadMessagesCommmand,
          Qt::UniqueConnection);

  // connect the signal from the message editor screen to the send-message
  // method
  connect(MessageChattingInUser, &ScreenChatting::signalSendMessage, this,
          [this, chatListInUser, MessageChattingInUser] {
            const auto &idx = chatListInUser->getCcurrentChatIndex();

            if (!idx.isValid())
              return;

            const auto &currentChatId = static_cast<size_t>(
              idx.data(ChatListModel::ChatIdRole).toLongLong());

            QString newMessageText;
            newMessageText =
              MessageChattingInUser->getScreenChattingNewMessageTextEdit()
                ->toPlainText();

            if (newMessageText.trimmed().isEmpty())
              return;

            if (currentChatId != 0)
              sendMessageCommmand(idx, currentChatId, newMessageText, false);
            else
              sendMessageCommmand(idx, currentChatId, newMessageText, true);

            if (!chatListInUser)
              return;

            auto selectMode = chatListInUser->getSelectionModel();
            if (!selectMode)
              return;

            selectMode->setCurrentIndex(selectMode->model()->index(0, 0),
                                        QItemSelectionModel::ClearAndSelect);
          });
}

void ScreenMainWork::clearChatListModelWithData() {
  if (auto sm = ui->mainWorkTabChatsList->getSelectionModel()) {
    QSignalBlocker b1(sm);
    auto clInUser = ui->mainWorkPageUserDataView->findChild<ScreenChatList *>(
      "ScreenUserDataChatsListWidget");
    QSignalBlocker b2(clInUser ? clInUser->getSelectionModel() : nullptr);

    _ChatListModel->clear();
  }
}

void ScreenMainWork::refillChatListModelWithData(bool allChats) {
  if (auto sm = ui->mainWorkTabChatsList->getSelectionModel()) {
    QSignalBlocker b1(sm);
    auto clInUser = ui->mainWorkPageUserDataView->findChild<ScreenChatList *>(
      "ScreenUserDataChatsListWidget");
    QSignalBlocker b2(clInUser ? clInUser->getSelectionModel() : nullptr);

    _ChatListModel->clear();
    fillChatListModelWithData(allChats);
  }
}

void ScreenMainWork::on_mainWorkChatUserTabWidget_currentChanged(int index) {
  if (index == 1) {
    // userData

    emit signalClearUserDataToLabels();

    ui->addUserToChatPushButton->setVisible(false);

    const auto userDataView =
      ui->mainWorkPageUserDataView->findChild<ScreenUserData *>(
        "ScreenUserDataUserDataWidget");

    ui->findLineEdit->clear();
    ui->findLineEdit->setEnabled(true);

    ui->findLineEdit->setPlaceholderText("Search...");

    ui->mainWorkRightStackedWidget->setCurrentIndex(1);

    fillUserListModelWithData();

    auto selectModelUsersList = ui->mainWorkUsersList->selectionModel();

    if (selectModelUsersList) {
      selectModelUsersList->setCurrentIndex(
        selectModelUsersList->model()->index(0, 0),
        QItemSelectionModel::ClearAndSelect);
    }

    const auto &userIdx = ui->mainWorkUsersList->currentIndex();

    const auto chatListInUser =
      ui->mainWorkPageUserDataView->findChild<ScreenChatList *>(
        "ScreenUserDataChatsListWidget");

    if (!chatListInUser)
      return;

    auto selectMode = chatListInUser->getSelectionModel();
    if (!selectMode)
      return;

    selectMode->setCurrentIndex(selectMode->model()->index(0, 0),
                                QItemSelectionModel::ClearAndSelect);

    if (userIdx.isValid()) {
      ui->mainWorkUsersList->setCurrentIndex(
        ui->mainWorkUsersList->model()->index(0, 0));
    }
  } else {
    // chatList

    QPalette paletteLineEdit = ui->findLineEdit->palette();
    paletteLineEdit.setColor(QPalette::PlaceholderText, QColor("#9AA0A6"));

    // ui->findLineEdit->setPalette(paletteLineEdit);

    ui->findLineEdit->setEnabled(false);
    ui->findLineEdit->setPlaceholderText("Search...");

    ui->mainWorkRightStackedWidget->setCurrentIndex(0);

    refillChatListModelWithData(true);
  }
  auto *lv = ui->mainWorkTabChatsList->findChild<QListView *>("chatListView");

  if (lv) {
    if (auto *m = lv->model(); m && m->rowCount() > 0) {
      const QModelIndex first = m->index(0, 0);
      lv->setCurrentIndex(first);
    }
  }
}

void ScreenMainWork::sendMessageCommmand(const QModelIndex idx,
                                         const std::size_t currentChatId,
                                         const QString &newMessageText,
                                         bool newChatBool) {
  if (newMessageText.trimmed().isEmpty())
    return;

  // build the message structure
  std::vector<std::shared_ptr<IMessageContent>> iMessageContent;
  TextContent textContent = newMessageText.toStdString();
  MessageContent<TextContent> messageContentText(textContent);
  iMessageContent.push_back(
    std::make_shared<MessageContent<TextContent>>(messageContentText));

  auto newMessageTimeStamp = getCurrentDateTimeInt();
  Message newMessage(iMessageContent, client_session_ptr_->getActiveUserCl(),
                     newMessageTimeStamp, 0);

  std::size_t newMessageId;

  //    if it is a new chat, call first
  if (newChatBool) {
    // created pointer to the new chat
    auto chat_ptr = std::make_shared<Chat>();

    // added the message to the chat
    chat_ptr->addMessageToChat(std::make_shared<Message>(newMessage),
                               client_session_ptr_->getActiveUserCl(), false);

    // fill in the recipients
    std::vector<UserDTO> participants;
    participants.clear();

    UserDTO userDTO;
    userDTO.login = client_session_ptr_->getActiveUserCl()->getLogin();
    participants.push_back(userDTO);

    for (int i = 0; i < _newChatUserListModel->rowCount(); ++i) {
      UserDTO userDTO;
      const auto &idx = _newChatUserListModel->index(i, 0);

      userDTO.login =
        idx.data(UserListModel::LoginRole).toString().toStdString();
      userDTO.userName =
        idx.data(UserListModel::NameRole).toString().toStdString();
      userDTO.passwordhash = "-1";
      userDTO.email =
        idx.data(UserListModel::EmailRole).toString().toStdString();
      userDTO.phone =
        idx.data(UserListModel::PhoneRole).toString().toStdString();
      userDTO.disable_reason =
        idx.data(UserListModel::DisableReasonRole).toString().toStdString();
      userDTO.is_active = idx.data(UserListModel::IsActiveRole).toBool();
      userDTO.disabled_at =
        idx.data(UserListModel::DisableAtRole).toULongLong();
      userDTO.ban_until = idx.data(UserListModel::BunUntilRole).toULongLong();

      participants.push_back(userDTO);
    }

    // bool ClientSession::CreateAndSendNewChatQt(std::shared_ptr<Chat>
    // &chat_ptr, std::vector<std::string> &participants, Message &newMessage)

    // send to the server and get the result
    bool result = client_session_ptr_->CreateAndSendNewChatCl(
      chat_ptr, participants, newMessage);

    // if the server response is not OK
    if (!result) {
      const auto time_stamp = QString::fromStdString(
        formatTimeStampToString(newMessageTimeStamp, true));
      const QString user_login = QString::fromStdString(
        client_session_ptr_->getActiveUserCl()->getLogin());
      const QString chat_id_str = "new chat";
      const QString event = "Message sending failed";
      const QString content_str = newMessageText;

      const QString log_line = QStringLiteral(
                                 "[%1]   [%2]   [%3]   [user=%4]   "
                                 "[chat_id=%5]   %6   Content: %8")
                                 .arg(time_stamp,
                                      "ERROR",      // Logging levels:
                                      "CHAT",       // Key modules for the chat
                                      user_login,   // User
                                      chat_id_str,  // User
                                      event,        // Event
                                      content_str); // message

      emit logger_ptr_->signalWriteLine(log_line);

      QMessageBox::warning(this, tr("Error!"),
                           tr("Cannot send the message."));
      return;
    } else {
      const auto time_stamp = QString::fromStdString(
        formatTimeStampToString(newMessageTimeStamp, true));
      const QString user_login = QString::fromStdString(
        client_session_ptr_->getActiveUserCl()->getLogin());
      const QString chat_id_str = QString::number(chat_ptr->getChatId());

      const auto &it = chat_ptr->getMessages().rbegin();
      const auto &msg = it->second->getMessageId();
      const QString msg_id_str = QString::number(msg);

      const QString event = "Message sending sucsessed";
      const QString content_str = newMessageText;

      const QString log_line =
        QStringLiteral(
          "[%1]   [%2]   [%3]   [user=%4]   [chat_id=%5]   [msg_Id=%6]   "
          "%7   Content: %8")
          .arg(time_stamp,
               "INFO",       // Logging levels:
               "CHAT",       // Key modules for the chat
               user_login,   // User
               chat_id_str,  //
               msg_id_str,   //
               event,        // Event
               content_str); // message

      emit logger_ptr_->signalWriteLine(log_line);
    }

    // replaced in the chats model
    _ChatListModel->setChatId(idx.row(), chat_ptr->getChatId());
  } // if newChatBool

  // if this is not a new chat
  else {
    auto chat_ptr =
      client_session_ptr_->getInstanceCl().getChatById(currentChatId);

    std::vector<std::shared_ptr<User>> recipients;
    for (const auto &participant : chat_ptr->getParticipants()) {
      auto user_ptr = participant._user.lock();
      if (user_ptr && user_ptr != client_session_ptr_->getActiveUserCl())
        recipients.push_back(user_ptr);
    }

    newMessageId = client_session_ptr_->createMessageCl(
      newMessage, chat_ptr,
      client_session_ptr_->getInstanceCl().getActiveUser());

    if (newMessageId == 0) {
      const auto time_stamp = QString::fromStdString(
        formatTimeStampToString(newMessageTimeStamp, true));
      const QString user_login = QString::fromStdString(
        client_session_ptr_->getActiveUserCl()->getLogin());
      const QString chat_id_str = QString::number(currentChatId);
      const QString msg_id_str = "new";
      const QString event = "Message sending failed";
      const QString content_str = newMessageText;

      const QString log_line =
        QStringLiteral(
          "[%1]   [%2]   [%3]   [user=%4]   [chat_id=%5]   [msg_Id=%6]   "
          "%7   Content: %8")
          .arg(time_stamp,
               "ERROR",      // Logging levels:
               "CHAT",       // Key modules for the chat
               user_login,   // User
               chat_id_str,  // User
               msg_id_str,   //
               event,        // Event
               content_str); // message

      emit logger_ptr_->signalWriteLine(log_line);

      QMessageBox::warning(this, tr("Error!"),
                           tr("Cannot send the message."));
      return;
    } else {
      const auto time_stamp = QString::fromStdString(
        formatTimeStampToString(newMessageTimeStamp, true));
      const QString user_login = QString::fromStdString(
        client_session_ptr_->getActiveUserCl()->getLogin());
      const QString chat_id_str = QString::number(currentChatId);

      const QString msg_id_str = QString::number(newMessageId);

      const QString event = "Message sending sucsessed";
      const QString content_str = newMessageText;

      const QString log_line =
        QStringLiteral(
          "[%1]   [%2]   [%3]   [user=%4]   [chat_id=%5]   [msg_Id=%6]   "
          "%7   Content: %8")
          .arg(time_stamp,
               "INFO",       // Logging levels:
               "CHAT",       // Key modules for the chat
               user_login,   // User
               chat_id_str,  //
               msg_id_str,   //
               event,        // Event
               content_str); // message

      emit logger_ptr_->signalWriteLine(log_line);
    }
  } // else - not a new chat

  _MessageModel->fillMessageItem(
    newMessageText,
    QString::fromStdString(
      client_session_ptr_->getActiveUserCl()->getLogin()),
    QString::fromStdString(
      client_session_ptr_->getActiveUserCl()->getUserName()),
    newMessageTimeStamp, newMessageId);

  // clear the source editor
  if (auto s = qobject_cast<ScreenChatting *>(sender()); s) {
    if (auto ed = s->getScreenChattingNewMessageTextEdit())
      ed->setText("");
  } else {
    ui->mainWorkPageChatting->getScreenChattingNewMessageTextEdit()->setText(
      "");
  }

  // replaced in the chats model
  _ChatListModel->setLastTime(idx.row(), newMessageTimeStamp);

  if (newChatBool) {
    slotMainWorkTransferrNewChatToMainChatList();

    ui->createNewChatPushButton->setEnabled(true);

    ui->mainWorkChatUserTabWidget->setTabEnabled(0, true);
    ui->mainWorkChatUserTabWidget->setCurrentIndex(0);

    ui->addUserToChatPushButton->setVisible(true);

    clearChatListModelWithData();
    clearMessageModelWithData();
    fillChatListModelWithData(true);
  } else
    refillChatListModelWithData(true);
}

void ScreenMainWork::resetCountUnreadMessagesCommmand() {
  // Since the user entered the chat and read everything, reset the new
  // messages count

  // took the row index
  const auto &idx = ui->mainWorkTabChatsList->getCcurrentChatIndex();

  // retrieved chatId
  if (!idx.isValid())
    return;

  const auto &currentChatId =
    static_cast<size_t>(idx.data(ChatListModel::ChatIdRole).toLongLong());
  if (currentChatId == 0)
    return;

  // retrieved the chat pointer
  const auto &chat_ptr =
    client_session_ptr_->getInstanceCl().getChatById(currentChatId);
  if (!chat_ptr)
    return;
  if (chat_ptr->getMessages().empty())
    return;

  // checked whether all messages are read
  const auto &lastMessageId =
    chat_ptr->getMessages().rbegin()->second->getMessageId();
  const auto &lastReadMessageId = chat_ptr->getLastReadMessageId(
    client_session_ptr_->getInstanceCl().getActiveUser());

  if (lastMessageId != lastReadMessageId) {
    chat_ptr->setLastReadMessageId(client_session_ptr_->getActiveUserCl(),
                                   lastMessageId);

    // sent a command to the server to set the last read message
    client_session_ptr_->sendLastReadMessageFromClientCl(chat_ptr,
                                                         lastMessageId);

    // replaced the unread count in the model and refreshed the list item
    _ChatListModel->setUnreadCount(idx.row(), 0);
  }
}

void ScreenMainWork::slotConnectUserListToChatListchange(bool status) {
  // get the selection model of the second chat list (chat list of a specific
  // user)
  auto selectMode = ui->mainWorkUsersList->selectionModel();

  if (!status)
    QObject::disconnect(_connUserListToChatListChange);
  else {
    // connection: index change when a contact is selected -> populate the
    // contact chat list window
    _connUserListToChatListChange = QObject::connect(
      selectMode, &QItemSelectionModel::currentChanged, this, [this](auto) {
        refillChatListModelWithData(false);
        const auto chatListInUser =
          ui->mainWorkPageUserDataView->findChild<ScreenChatList *>(
            "ScreenUserDataChatsListWidget");
        if (!chatListInUser)
          return;
        auto selectMode = chatListInUser->getSelectionModel();
        if (!selectMode)
          return;

        selectMode->setCurrentIndex(selectMode->model()->index(0, 0),
                                    QItemSelectionModel::ClearAndSelect);
      });
  }
}

void ScreenMainWork::on_createNewChatPushButton_clicked() {
  // "contact chats" screen
  const auto chatListInUser =
    ui->mainWorkPageUserDataView->findChild<ScreenChatList *>(
      "ScreenUserDataChatsListWidget");
  if (!chatListInUser)
    return;

  // pointer to the object containing the messages window in single-contact
  // chat mode (from the address book)
  auto MessageChattingInUser =
    ui->mainWorkPageUserDataView->findChild<ScreenChatting *>(
      "ScreenUserDataMessagesListWidget");
  if (!MessageChattingInUser)
    return;

  _newChatUserListModel = new UserListModel();

  // connection: index change when a contact is selected -> populate the contact
  // chat list window
  slotConnectUserListToChatListchange(false);

  ui->createNewChatPushButton->setEnabled(false);

  ui->mainWorkChatUserTabWidget->setTabEnabled(0, false);

  ui->addUserToChatPushButton->setVisible(true);

  slotAddUserToNewChatList();

  QString login;

  if (_newChatUserListModel->rowCount() == 0)
    login = "";
  else
    login = _newChatUserListModel->getItem(0).Login;

  _newChatMode = true;

  clearChatListModelWithData();
  clearMessageModelWithData();

  emit signalStartNewChat(true);
}

void ScreenMainWork::slotAddUserToNewChatList() {
  const auto &selectModel = ui->mainWorkUsersList->selectionModel();
  const auto &idx = selectModel->currentIndex();

  if (idx.isValid()) {
    const auto &isActive = idx.data(UserListModel::IsActiveRole).toBool();
    const auto &bunTo = idx.data(UserListModel::BunUntilRole).toLongLong();

    if (!isActive) {
      QMessageBox::warning(this, "Message", "Contact is blocked.");
      return;
    }
    if (bunTo != 0 && bunTo > getCurrentDateTimeInt()) {
      QMessageBox::warning(this, "Message", "User is banned.");
      return;
    }

    const auto &value = idx.data(UserListModel::LoginRole).toString();

    const auto element = _userListModel->getItem(idx.row());
    _newChatUserListModel->appendItem(element);

    emit signalAddContactToNewChat(_newChatUserListModel, value);
  }
}

void ScreenMainWork::on_addUserToChatPushButton_clicked() {
  slotAddUserToNewChatList();
}

void ScreenMainWork::on_findLineEdit_textChanged(const QString &arg1) {
  slotFindContactsByPart();
}

void ScreenMainWork::on_findLineEdit_editingFinished() {
  _startFind = true;
}

void ScreenMainWork::on_mainWorkUsersList_doubleClicked(
  const QModelIndex &index) {
  if (_newChatMode)
    slotAddUserToNewChatList();
}
void ScreenMainWork::on_logOutPushButton_clicked() {
  emit signalLogOut();
}

void ScreenMainWork::on_ProfilePushButton_clicked() {
  emit signalShowProfile();
}

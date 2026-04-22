#include "chat/chat.h"
#include "exceptions_cpp/validation_exception.h"
#include "user/user.h"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <memory>

// getters

const std::size_t &Chat::getChatId() const { return _chatId; };

std::set<std::size_t> &Chat::getMessageIdMap() { return _messageIdMap; }
const std::set<std::size_t> &Chat::getMessageIdMap() const { return _messageIdMap; }

const std::unordered_map<std::size_t, int64_t> &Chat::getMessageIdToTimeStamp() const { return _messageIdToTimeStamp; };

const std::multimap<int64_t, std::shared_ptr<Message>> &Chat::getMessages() const { return _messages; }
std::multimap<int64_t, std::shared_ptr<Message>> &Chat::getMessages() { return _messages; }

std::unordered_map<std::string, std::unordered_set<std::size_t>> &Chat::getDeletedMessagesMap() {
  return _deletedMessagesMap;
}
const std::unordered_map<std::string, std::unordered_set<std::size_t>> &Chat::getDeletedMessagesMap() const {
  return _deletedMessagesMap;
}

const std::vector<Participant> &Chat::getParticipants() const { return _participants; }

std::size_t Chat::getLastReadMessageId(const std::shared_ptr<User> &user) const {

  auto it = _lastReadMessageMap.find(user->getLogin());
  try {
    if (it != _lastReadMessageMap.end()) {
      return it->second;
    } else
      throw exc::UserNotInListException();
  } catch (const exc::UserNotInListException &ex) {
    std::cout << " ! " << ex.what() << " getLastReadMessageId" << std::endl;
    return 0;
  }
}

// setters
void Chat::setMessageIdMap(const std::size_t &messageId) { _messageIdMap.insert(messageId); }

void Chat::setChatId(const std::size_t &chatId) { _chatId = chatId; }
//
//
//
void Chat::setMessageIdToTimeStamp(const std::size_t &messageId, const int64_t &timeStamp) {
  _messageIdToTimeStamp.insert({messageId, timeStamp});
}
//
//
//
void Chat::addParticipant(const std::shared_ptr<User> &user, std::size_t lastReadMessageIndex, bool deletedFromChat) {

  Participant participant;
  participant._user = user;
  participant._deletedFromChat = deletedFromChat;
  _participants.push_back(participant);
  setLastReadMessageId(user, lastReadMessageIndex);
}
//
//
//
void Chat::addMessageToChat(const std::shared_ptr<Message> &message, const std::shared_ptr<User> &sender,
                            const bool &isServerSession) {

  std::size_t messageId = 0;

  messageId = message->getMessageId();

  const auto &timeStamp = message->getTimeStamp();

  // добавили само сообщение в multimap
  _messages.insert({timeStamp, message});

  // добавили messageId в set
  setMessageIdMap(messageId);

  // добавляем соответствие номера сообщения и метки времени
  setMessageIdToTimeStamp(messageId, timeStamp);

  // корректируем последнее прочитанное сообщение для отправителя

  setLastReadMessageId(sender, messageId);
}

void Chat::setLastReadMessageId(const std::shared_ptr<User> &user, std::size_t newLastReadMessageId) {

  _lastReadMessageMap.insert_or_assign(user->getLogin(), newLastReadMessageId);
}

bool Chat::setDeletedMessageMap(const std::string &userLogin, const std::size_t &deletedMessageId) {

  auto it = _deletedMessagesMap.find(userLogin);

  if (it == _deletedMessagesMap.end()) {

    std::unordered_set<std::size_t> newSet;
    newSet.insert(deletedMessageId);
    _deletedMessagesMap.insert({userLogin, newSet});

  } else {
    const auto &it2 = it->second.find(deletedMessageId);
    if (it2 == it->second.end())
      it->second.insert(deletedMessageId);
    else
      return false;
  }
  return true;
}

std::size_t Chat::getUnreadMessageCount(const std::shared_ptr<User> &user_ptr) {

  const auto &login = user_ptr->getLogin();

  // проверяем есть ли запись о последнем прочитанном сообщении
  auto it = _lastReadMessageMap.find(login);

  // если пользователь еще ничего не читал
  if (it == _lastReadMessageMap.end())
    return _messages.size();

  // получаем MessageId последнего непрочитанного сообщения
  auto lastReadMessageId = it->second;

  // проверяем наличие временной метки
  const auto itTimeStamp = _messageIdToTimeStamp.find(lastReadMessageId);
  if (itTimeStamp == _messageIdToTimeStamp.end())
    return _messages.size();

  const auto timeStamp = itTimeStamp->second;

  // upper_bound возвращает первый элемент с timestamp > прочитанного
  auto itUnread = _messages.upper_bound(timeStamp);
  return static_cast<std::size_t>(std::distance(itUnread, _messages.end()));

  //   std::cout << std::endl << "DEBUG: getUnreadMessageCount. lastReadMessageId = " << lastReadMessageId << std::endl;

  //   std::cout << " timeStamp = " << timeStamp << " = " << formatTimeStampToString(timeStamp, true) << std::endl;

  //   std::cout << "distance = " << dist << std::endl;
}

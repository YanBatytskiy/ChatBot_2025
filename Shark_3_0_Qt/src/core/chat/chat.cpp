#include "chat/chat.h"
#include "exceptions_cpp/validation_exception.h"
#include "user/user.h"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <memory>

// getters

const std::size_t &Chat::getChatId() const {
  return _chatId;
};

std::set<std::size_t> &Chat::getMessageIdMap() {
  return _messageIdMap;
}
const std::set<std::size_t> &Chat::getMessageIdMap() const {
  return _messageIdMap;
}

const std::unordered_map<std::size_t, int64_t> &Chat::getMessageIdToTimeStamp() const {
  return _messageIdToTimeStamp;
};

const std::multimap<int64_t, std::shared_ptr<Message>> &Chat::getMessages() const {
  return _messages;
}
std::multimap<int64_t, std::shared_ptr<Message>> &Chat::getMessages() {
  return _messages;
}

std::unordered_map<std::string, std::unordered_set<std::size_t>> &Chat::getDeletedMessagesMap() {
  return _deletedMessagesMap;
}
const std::unordered_map<std::string, std::unordered_set<std::size_t>> &Chat::getDeletedMessagesMap() const {
  return _deletedMessagesMap;
}

const std::vector<Participant> &Chat::getParticipants() const {
  return _participants;
}

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
void Chat::setMessageIdMap(const std::size_t &messageId) {
  _messageIdMap.insert(messageId);
}

void Chat::setChatId(const std::size_t &chatId) {
  _chatId = chatId;
}
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

  // Inserted the message itself into the multimap
  _messages.insert({timeStamp, message});

  // Added messageId to the set
  setMessageIdMap(messageId);

  // Add the mapping between the message number and its timestamp
  setMessageIdToTimeStamp(messageId, timeStamp);

  // Adjust the last-read message for the sender

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

  // Check whether there is a record of the last-read message
  auto it = _lastReadMessageMap.find(login);

  // If the user has not read anything yet
  if (it == _lastReadMessageMap.end())
    return _messages.size();

  // Get the messageId of the last unread message
  auto lastReadMessageId = it->second;

  // Check whether a timestamp is present
  const auto itTimeStamp = _messageIdToTimeStamp.find(lastReadMessageId);
  if (itTimeStamp == _messageIdToTimeStamp.end())
    return _messages.size();

  const auto timeStamp = itTimeStamp->second;

  // upper_bound returns the first element whose timestamp is greater than the one read
  auto itUnread = _messages.upper_bound(timeStamp);
  return static_cast<std::size_t>(std::distance(itUnread, _messages.end()));

  // Optional unread-counter trace kept below for manual checks.
  // std::cout << std::endl << "DEBUG: getUnreadMessageCount. lastReadMessageId = " << lastReadMessageId << std::endl;

  // std::cout << " timeStamp = " << timeStamp << " = " << formatTimeStampToString(timeStamp, true) << std::endl;

  // std::cout << "distance = " << dist << std::endl;
}

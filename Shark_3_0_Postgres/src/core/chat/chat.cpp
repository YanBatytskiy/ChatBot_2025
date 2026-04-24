#include "core/chat/chat.h"
#include "core/exception/validation_exception.h"
#include "core/user/user.h"
#include <algorithm>
#include <iostream>
#include <iterator>
#include <memory>
#include <cstdint>

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

std::size_t &Chat::getNextMessageIdClient() {
  return _nextMessageId;
}
const std::size_t &Chat::getNextMessageIdClient() const {
  return _nextMessageId;
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

bool Chat::getUserDeletedFromChat(const std::shared_ptr<User> &user) const {
  const auto &participants = _participants;
  auto it = std::find_if(participants.begin(), participants.end(), [&user](const Participant &participant) {
    auto user_ptr = participant._user.lock();
    return user_ptr && (user_ptr == user);
  });
  try {
    if (it != participants.end())
      return it->_deletedFromChat;
    else
      throw exc::UserNotInListException();
  } catch (const exc::ValidationException &ex) {
    std::cout << " ! " << ex.what() << " Try again." << std::endl;
    return true;
  }
}

// setters
void Chat::setMessageIdMap(const std::size_t &messageId) {
  _messageIdMap.insert(messageId);
}

void Chat::setNextMessageIdClient(const std::size_t &nextMessageId) {
  _nextMessageId = nextMessageId;
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
void Chat::updateMessageIdToTimeStamp(const std::size_t &oldMessageId, const std::size_t &newMessageId) {
  auto it = _messageIdToTimeStamp.find(oldMessageId);
  if (it != _messageIdToTimeStamp.end()) {
    _messageIdToTimeStamp[newMessageId] = it->second;
    _messageIdToTimeStamp.erase(it); // remove the old one if needed
  }
}
//
//
//
std::int64_t Chat::getTimeStampForLastMessage(const std::size_t &messageId) const {
  return _messageIdToTimeStamp.find(messageId)->second;
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

  if (isServerSession) {
    messageId = createNewMessageId(isServerSession);
    message->setMessageId(messageId);
  } else {
    messageId = message->getMessageId();
  }

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

void Chat::setUserDeletedFromChat(const std::shared_ptr<User> &user) {
  auto &participants = _participants;
  auto it = std::find_if(participants.begin(), participants.end(), [&user](const Participant &participant) {
    auto user_ptr = participant._user.lock();
    return user_ptr && (user_ptr == user);
  });
  try {
    if (it != participants.end())
      it->_deletedFromChat = true;
    else
      throw exc::UserNotInListException();
  } catch (const exc::ValidationException &ex) {
    std::cout << " ! " << ex.what() << " Try again." << std::endl;
  }
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

// utilities

bool Chat::clearChat() {
  _chatId = 0;
  _participants.clear();
  _messages.clear();
  _messageIdMap.clear();
  _nextMessageId = 1;
  _lastReadMessageMap.clear();
  _messageIdToTimeStamp.clear();
  _deletedMessagesMap.clear();
  return true;
}

std::size_t Chat::createNewMessageId([[maybe_unused]] bool isServerStatus) {

  if (_messageIdMap.empty())
    return 1;
  return *std::prev(_messageIdMap.end()) + 1;
}

std::size_t Chat::getUnreadMessageCount(const std::shared_ptr<User> &user_ptr) {

  const auto &login = user_ptr->getLogin();

  // Check whether a last-read record exists
  auto it = _lastReadMessageMap.find(login);

  // If the user has not read anything yet
  if (it == _lastReadMessageMap.end())
    return _messages.size();

  // Get the MessageId of the last read message
  auto lastReadMessageId = it->second;

  // Check whether the timestamp exists
  const auto itTimeStamp = _messageIdToTimeStamp.find(lastReadMessageId);
  if (itTimeStamp == _messageIdToTimeStamp.end())
    return _messages.size();

  const auto timeStamp = itTimeStamp->second;

  // upper_bound returns the first element with timestamp > the last read one
  auto itUnread = _messages.upper_bound(timeStamp);
  return static_cast<std::size_t>(std::distance(itUnread, _messages.end()));

  //   std::cout << std::endl << "DEBUG: getUnreadMessageCount. lastReadMessageId = " << lastReadMessageId << std::endl;

  //   std::cout << " timeStamp = " << timeStamp << " = " << formatTimeStampToString(timeStamp, true) << std::endl;

  //   std::cout << "distance = " << dist << std::endl;
}
//
//
//
// Print a chat
void Chat::printChat(const std::shared_ptr<User> &currentUser) {

  if (_messages.empty()) {
    std::cout << "No messages." << std::endl;
  } else {

    // auto test = this->getChatId();

    std::cout << std::endl
              << "Here is your chat, chatId: " << this->getChatId() << ". It has a total of " << _messages.size()
              << " message(s). ";
    std::cout << "\033[32m"; // green
    std::cout << "Unread among them - " << getUnreadMessageCount(currentUser) << std::endl;
    std::cout << "\033[0m";

    // Print the participant list excluding the active user
    std::cout << std::endl
              << "Chat participants Name/Login: " << std::endl;

    // Iterate over chat participants
    for (const auto &participant : this->getParticipants()) {
      auto user_ptr = participant._user.lock();
      if (user_ptr) {
        if (user_ptr != currentUser) {
          std::cout << user_ptr->getUserName() << "/" << user_ptr->getLogin() << "; ";
        };
      } else {
        std::cout << "removed user";
      }
    }

    std::cout << std::endl;

    // Print the messages themselves
    for (const auto &message : _messages)
      //   {
      message.second->printMessage(currentUser);
    // }
  }
}

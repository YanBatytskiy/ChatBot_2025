#include "chat_system.h"
#include "chat/chat.h"
#include "dto/dto_struct.h"
#include "exceptions_cpp/login_exception.h"
#include "exceptions_cpp/validation_exception.h"
#include "system/system_function.h"
#include "user/user_chat_list.h"
#include <cstdint>
#include <iostream>
#include <iterator>
#include <memory>

#include <arpa/inet.h>
#include <netinet/in.h>

/**
 * @brief Default constructor for ChatSystem.
 */
ChatSystem::ChatSystem() {}

// getters

std::shared_ptr<Chat> ChatSystem::getChatById(std::size_t chatId) const {
  auto it = _chatIdChatMap.find(chatId);
  if (it != _chatIdChatMap.end())
    return it->second;
  return nullptr;
}

const std::vector<std::shared_ptr<User>> &ChatSystem::getUsers() const {
  return _users;
}

const std::shared_ptr<User> &ChatSystem::getActiveUser() const {
  return _activeUser;
}

const std::unordered_map<std::string, std::shared_ptr<User>> &
ChatSystem::getLoginUserMap() const {
  return _loginUserMap;
}

// setters

void ChatSystem::setIsServerStatus(const bool &serverStatus) {
  _isServerStatus = serverStatus;
}

void ChatSystem::setActiveUser(const std::shared_ptr<User> &user) {
  _activeUser = user;
}

void ChatSystem::addUserToSystem(std::shared_ptr<User> &user) {
  _users.push_back(user);
  _loginUserMap.insert({user->getLogin(), user});
}

bool ChatSystem::addChatToInstance(const std::shared_ptr<Chat> &chat_ptr) {

  bool result = true;
  const std::size_t chatId = chat_ptr->getChatId();
  try {
    if (!chat_ptr)
      throw exc::ChatNotFoundException();

    _activeUser->getUserChatList()->addChatToChatList(chat_ptr);

  } // try
  catch (const exc::ChatNotFoundException &ex) {
    std::cout << " ! " << ex.what() << std::endl;
    return false;
  }
  _chats.push_back(chat_ptr);
  _chatIdChatMap.insert({chatId, chat_ptr});

  return true;
}

void ChatSystem::clear_chat_system() {
  _activeUser.reset();
  _users.clear();
  _chats.clear();
  _loginUserMap.clear();
  _chatIdChatMap.clear();
  _isServerStatus = false;
}

std::shared_ptr<User>
ChatSystem::findUserByLogin(const std::string &userLogin) const {

  auto it = this->getLoginUserMap().find(userLogin);
  if (it == this->getLoginUserMap().end())
    return nullptr;
  else
    return it->second;
}

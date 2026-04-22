#include "chat_system.h"
#include "chat/chat.h"
#include "dto/dto_struct.h"
#include "exception/login_exception.h"
#include "exception/validation_exception.h"
#include "system/system_function.h"
#include "user/user_chat_list.h"
#include <iostream>
#include <memory>
#include <cstdint>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#endif


/**
 * @brief Default constructor for ChatSystem.
 */
ChatSystem::ChatSystem() {}

// getters

const bool &ChatSystem::getIsServerStatus() const { return _isServerStatus; }

std::size_t &ChatSystem::getNextChatId() { return _nextChatIdClient; }
const std::size_t &ChatSystem::getNextChatId() const { return _nextChatIdClient; }

std::set<std::size_t> &ChatSystem::getFreeChatServerIdSet() { return _freeChatServerIdSet; }
const std::set<std::size_t> &ChatSystem::getFreeChatServerIdSet() const { return _freeChatServerIdSet; }

std::shared_ptr<Chat> ChatSystem::getChatById(std::size_t chatId) const {
  auto it = _chatIdChatMap.find(chatId);
  if (it != _chatIdChatMap.end())
    return it->second;
  return nullptr;
}

std::deque<std::pair<PacketDTO, std::string>> &ChatSystem::getPacketReceivedDeque() { return _packetReceivedDeque; }

std::deque<std::pair<PacketListDTO, std::string>> &ChatSystem::getPacketForSendDeque() { return _packetForSendDeque; }

/**
 * @brief Gets the list of users.
 * @return Const reference to the vector of users.
 */
const std::vector<std::shared_ptr<User>> &ChatSystem::getUsers() const { return _users; }

/**
 * @brief Gets the list of chats.
 * @return Const reference to the vector of chats.
 */
const std::vector<std::shared_ptr<Chat>> &ChatSystem::getChats() const { return _chats; }

/**
 * @brief Gets the active user.
 * @return Const reference to the active user.
 */
const std::shared_ptr<User> &ChatSystem::getActiveUser() const { return _activeUser; }

/**
 * @brief Gets the login user map.
 * @return Const reference to the unordered map.
 */
const std::unordered_map<std::string, std::shared_ptr<User>> &ChatSystem::getLoginUserMap() const {
  return _loginUserMap;
}

// setters

void ChatSystem::setIsServerStatus(const bool &serverStatus) { _isServerStatus = serverStatus; }

void ChatSystem::setNextChatIdClient(const std::size_t &nextChatId) { _nextChatIdClient = nextChatId; }

void ChatSystem::setFreeChatServerId(const std::size_t &freeChatId) { _freeChatServerIdSet.insert(freeChatId); }

void ChatSystem::moveToFreeChatIdSrv(std::size_t chatId) {
  if (_isServerStatus)
    _freeChatServerIdSet.insert(chatId);
}

void ChatSystem::releaseFreeChatIdSrv(std::size_t chatId) {
  if (_isServerStatus)
    _freeChatServerIdSet.erase(chatId);
}

/**
 * @brief Sets the active user.
 * @param user shared pointer to the user to set as active.
 */
void ChatSystem::setActiveUser(const std::shared_ptr<User> &user) { _activeUser = user; }

/**
 * @brief Adds a user to the system.0``
 * @param user shared pointer to the user to add.
 */
void ChatSystem::addUserToSystem(std::shared_ptr<User> &user) {
  _users.push_back(user);
  _loginUserMap.insert({user->getLogin(), user});
}

/**
 * @brief Adds a chat to the system.
 * @param chat shared pointer to the chat to add.
 */
bool ChatSystem::addChatToInstance(const std::shared_ptr<Chat> &chat_ptr) {

  if (!chat_ptr)
    throw exc::ChatNotFoundException();

  const std::size_t chatId = chat_ptr->getChatId();
  bool result = true;

  // чтобы сделать откат
  std::vector<std::shared_ptr<User>> addParticipantsList;

  try {
    if (_isServerStatus) {
      for (const auto &participant : chat_ptr->getParticipants()) {
        if (auto user_ptr = participant._user.lock()) {

          // добавление чата в чат-лист
          auto userChatList = user_ptr->getUserChatList();

          if (userChatList) {
            userChatList->addChatToChatList(chat_ptr);
            addParticipantsList.push_back(user_ptr);
          } else
            throw exc::ChatListNotFoundException(user_ptr->getLogin());
        } // if user_ptr
        else
          throw exc::UserNotFoundException();
      }
    } else
      _activeUser->getUserChatList()->addChatToChatList(chat_ptr);
  } // try
  catch (const exc::ChatListNotFoundException &ex) {
    std::cout << " ! " << ex.what() << std::endl;
    result = false;
  } catch (const exc::ChatNotFoundException &ex) {
    std::cout << " ! " << ex.what() << std::endl;
    result = false;
  } catch (const exc::UserNotFoundException &ex) {
    std::cout << " ! " << ex.what() << std::endl;
    result = false;
  }
  if (result) {
    _chats.push_back(chat_ptr);
    _chatIdChatMap.insert({chatId, chat_ptr});
  } else {
    for (const auto &participant : addParticipantsList) {

      auto userChatList = participant->getUserChatList();

      userChatList->deleteChatFromList(chat_ptr);
    }
    addParticipantsList.clear();
  }
  return result;
}
/**
 * @brief Removes a user from the system (not implemented).
 * @param user shared pointer to the user to remove.
 */
void ChatSystem::eraseUser(const std::shared_ptr<User> &user) {
  // доделать
}

// utilities
std::size_t ChatSystem::createNewChatId(const std::shared_ptr<Chat> &chat_ptr) {

  // если это клиентская сессия
  if (!_isServerStatus) {

    std::size_t nextChatId = _nextChatIdClient;

    while (true) {
      const auto &it = _chatIdChatMap.find(nextChatId);

      if (it != _chatIdChatMap.end())
        ++nextChatId;
      else {
        _nextChatIdClient = nextChatId + 1;
        _chatIdChatMap.insert({nextChatId, chat_ptr});
        return nextChatId;
      }
    }
  } // if
  else {
    if (_freeChatServerIdSet.empty()) {
      int result;
      if (_chatIdChatMap.empty())
        result = 1;
      else
        result = _chatIdChatMap.rbegin()->first + 1;

      _chatIdChatMap.insert({result, chat_ptr});
      return result;
    } else {
      auto value = *_freeChatServerIdSet.begin();
      _freeChatServerIdSet.erase(_freeChatServerIdSet.begin());
      return value;
    }
  }
}

/**
 * @brief Finds users matching a search string.
 * @param users Vector to store found users.
 * @param textToFind Search string to match against user names or logins.
 * @details Performs case-insensitive search for users, excluding the active
 * user.
 */
std::vector<std::shared_ptr<User>> ChatSystem::findUserByTextPart(
    const std::string &textToFind) const { // поиск пользователя

  std::vector<std::shared_ptr<User>> foundUsers;

  std::string textToFindLower = TextToLower(textToFind);

  // перебираем всех пользователей в векторе
  for (const auto &user : _users) {

    if (user == _activeUser)
      continue;

    std::string LowerLogin = TextToLower(user->getLogin());
    std::string LowerName = TextToLower(user->getUserName());

    if (LowerLogin.find(textToFindLower) != std::string::npos || LowerName.find(textToFindLower) != std::string::npos)
      foundUsers.push_back(user);
  }
  return foundUsers;
}

/**
 * @brief Displays the list of users.
 * @param showActiveUser True to include the active user in the list.
 * @return The index of the active user in the list.
 * @details Prints user names and logins, excluding the active user if
 * showActiveUser is false.
 */
std::size_t ChatSystem::showUserList(const bool showActiveUser) { // вывод на экрын списка пользователей
  std::cout << "Список пользователей:" << std::endl;
  size_t index = 1;
  size_t returnIndex = std::string::npos;
  for (const auto &user : _users) {
    if (user == _activeUser) {
      returnIndex = index - 1;
    }

    if (!showActiveUser && user == _activeUser)
      continue;
    std::cout << index << ".  Имя - " << user->getUserName() << ", логин - " << user->getLogin() << ";" << std::endl;
    ++index;
  }
  return returnIndex;
}

std::shared_ptr<User> ChatSystem::findUserByLogin(const std::string &userLogin) const {

  auto it = this->getLoginUserMap().find(userLogin);
  if (it == this->getLoginUserMap().end())
    return nullptr;
  else
    return it->second;
}

const std::shared_ptr<User> ChatSystem::RqFrClientCheckLoginExists(const std::string &login) const {

  auto it = this->_loginUserMap.find(login);

  if (it != this->_loginUserMap.end())
    return it->second;
  else
    return nullptr;
}

bool ChatSystem::checkPasswordValidForUser(const std::string &passwordHash, const std::string &userLogin) {

  auto user = findUserByLogin(userLogin);

  if (user == nullptr)
    return false;

  const auto &check = user->getPasswordHash();

  return passwordHash == user->getPasswordHash();
}
#include "user.h"
#include "chat/chat.h"
#include "exception/validation_exception.h"
#include "message/message.h"
#include "system/date_time_utils.h"
#include "user_chat_list.h"
#include <cstddef>
#include <iostream>
#include <iterator>

User::User(const UserData &userData) : _userData(userData) {}

/**
 * @brief Assigns a chat list to the user.
 * @param userChats shared pointer to the user's chat list.
 */
void User::createChatList() {
  _userChatList = std::make_shared<UserChatList>(shared_from_this());
}

void User::createChatList(const std::shared_ptr<UserChatList> &userChatList) {
  _userChatList = userChatList;
}

/**
 * @brief Gets the user's login.
 * @return The user's login string.
 */
std::string User::getLogin() const {
  return _userData._login;
}

/**
 * @brief Gets the user's display name.
 * @return The user's display name string.
 */
std::string User::getUserName() const {
  return _userData._userName;
}

/**
 * @brief Gets the user's passwordHash.
 * @return The user's passwordHash string.
 */
std::string User::getPasswordHash() const {
  return _userData._passwordHash;
}

/**
 * @brief Gets the user's email.
 * @return The user's Email string.
 */
std::string User::getEmail() const {
  return _userData._email;
}

/**
 * @brief Gets the user's phone.
 * @return The user's phone string.
 */
std::string User::getPhone() const {
  return _userData._phone;
}

/**
 * @brief Gets the user's chat list.
 * @return shared pointer to the user's chat list.
 */
std::shared_ptr<UserChatList> User::getUserChatList() const {
  return _userChatList;
}

/**
 * @brief Sets the user's login.
 * @param login The new login string.
 */
void User::setLogin(const std::string &login) {
  _userData._login = login;
}

/**
 * @brief Sets the user's display name.
 * @param userName The new display name string.
 */
void User::setUserName(const std::string &userName) {
  _userData._userName = userName;
}

/**
 * @brief Sets the user's passwordHash.
 * @param passwordHash The new passwordHash string.
 */
void User::setPassword(const std::string &passwordHash) {
  _userData._passwordHash = passwordHash;
}

/**
 * @brief Sets the user's email.
 * @param email The new email string.
 */
void User::setEmail(const std::string &email) {
  _userData._email = email;
}

/**
 * @brief Sets the user's phone.
 * @param phone The new phone string.
 */
void User::setPhone(const std::string &phone) {
  _userData._phone = phone;
}

/**
 * @brief Checks if the provided passwordHash matches the user's passwordHash.
 * @param passwordHash The passwordHash to check.
 * @return True if the passwordHash matches, false otherwise.
 */
bool User::checkPassword(const std::string &passwordHash) const {
  return (passwordHash == getPasswordHash());
}

/**
 * @brief Checks if the provided login matches the user's login.
 * @param login The login to check.
 * @return True if the login matches, false otherwise.
 */
bool User::RqFrClientCheckLogin(const std::string &login) const {
  return (getLogin() == login);
}

/**
 * @brief Displays the user's data.
 * @details Prints the user's name, login, and passwordHash.
 */
void User::showUserData() const {
  std::cout << "Name: " << getUserName() << ", Login: " << getLogin() << std::endl;
}

/**
 * @brief Displays the user's data for init.
 * @details Prints the user's name, login, and passwordHash.
 */
void User::showUserDataInit() const {
  std::cout << "Name: " << getUserName() << ", Login: " << getLogin();
}

/**
 * @brief Prints the user's chat list.
 * @param user shared pointer to the user whose chat list is to be printed.
 * @throws UnknownException If the message vector of a chat is empty.
 * @details Displays each chat with participant details, last message timestamp,
 * and unread message count.
 * @note The rendering path still needs dedicated handling for deleted users and
 * unread-message counters in the list view.
 */
void User::printChatList(const std::shared_ptr<User> &user) const {
  // Get the chat list
  const auto &chatList = user->getUserChatList()->getChatFromList();

  if (chatList.empty()) {
    std::cout << "User " << user->getUserName() << " has no chats." << std::endl;
    return;
  }
  std::cout << std::endl
            << "Total chats = " << user->getUserChatList()->getChatFromList().size() << ". Chat list for user "
            << user->getUserName() << " :" << std::endl;

  std::size_t index = 1;

  // Iterate over chats in the list
  for (const auto &weakChat : chatList) {

    if (auto chat_ptr = weakChat.lock()) {

      std::cout << std::endl;
      std::cout << index << ". chatId: " << chat_ptr->getChatId() << ", Total messages: ";

      const auto &messages = chat_ptr->getMessages();
      try {
        if (messages.empty())
          throw exc::MessagesNotFoundException();
        else
          std::cout << messages.size() << ", ";

      } catch (const exc::ValidationException &ex) {
        std::cout << " ! " << ex.what() << std::endl;
      }

      // Iterate over chat participants
      std::cout << "Name/Login: ";
      for (const auto &participant : chat_ptr->getParticipants()) {
        auto user_ptr = participant._user.lock();

        if (user_ptr) {
          if (user_ptr != user) {
            std::cout << user_ptr->getUserName() << "/" << user_ptr->getLogin() << "; ";
          }
        } // if (user_ptr)
        else {
          std::cout << " removed user., ";
        }
      } // for participant
      // Print the date and time of the last message
      const auto &lastMessage_ptr = std::prev(messages.end())->second;
      const auto &timeStampLastMessage = lastMessage_ptr->getTimeStamp();

      std::cout << "Last message - " << formatTimeStampToString(timeStampLastMessage, true) << ". ";

      // Print the number of new messages

      std::cout << "new messages - " << chat_ptr->getUnreadMessageCount(user) << std::endl;
    } // if auto chat_ptr
    else {
      std::cout << "Chat removed." << std::endl;
    }
    ++index;

  } // for (const auto &weakChat
}

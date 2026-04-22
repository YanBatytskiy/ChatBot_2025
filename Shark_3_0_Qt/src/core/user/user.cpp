#include "user.h"
// #include "exceptions_cpp/validation_exception.h"
// #include "system/date_time_utils.h"
#include "user_chat_list.h"
#include <cstddef>
#include <cstdint>
// #include <iostream>
// #include <iterator>
#include <memory>
// #include <ostream>

User::User(const UserData &userData) : _userData(userData) {}

/**
 * @brief Assigns a chat list to the user.
 * @param userChats shared pointer to the user's chat list.
 */
void User::createChatList() { _userChatList = std::make_shared<UserChatList>(shared_from_this()); }

void User::createChatList(const std::shared_ptr<UserChatList> &userChatList) { _userChatList = userChatList; }

std::string User::getLogin() const { return _userData._login; }

std::string User::getUserName() const { return _userData._userName; }

std::string User::getPasswordHash() const { return _userData._passwordHash; }

std::string User::getEmail() const { return _userData._email; }

std::string User::getPhone() const { return _userData._phone; }

bool User::getIsActive() const { return _userData._isActive; }

std::int64_t User::getDisabledAt() const { return _userData._disabledAt; }

std::int64_t User::getBunUntil() const { return _userData._bunUntil; }

std::shared_ptr<UserChatList> User::getUserChatList() const { return _userChatList; }

void User::setPassword(const std::string &passwordHash) { _userData._passwordHash = passwordHash; }

bool User::RqFrClientCheckLogin(const std::string &login) const { return (getLogin() == login); }

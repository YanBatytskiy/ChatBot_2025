#pragma once

#include <memory>
#include <string>
#include <cstdint>

class UserChatList;

/**
 * @brief Structure to store user input data for registration or login.
 */
struct UserData {
  std::string _login;        ///< User login.
  std::string _passwordHash; ///< User passwordHash.
  std::string _userName;     ///< User's display name.
  std::string _email;        ///< User's email.
  std::string _phone;        ///< User's phone.
  std::string _disableReason;
  bool _isActive;
  std::int64_t _disabledAt;
  std::int64_t _bunUntil;

  UserData() = default;

  UserData(const std::string &login, const std::string &name, const std::string &passwordHash, const std::string &email,
           const std::string &phone, const std::string &disableReason, bool isActive, const std::size_t disabledAt,
           const std::size_t banUntil)
      : _login(login), _passwordHash(passwordHash), _userName(name), _email(email), _phone(phone),
        _disableReason(disableReason), _isActive(isActive), _disabledAt(disabledAt), _bunUntil(banUntil){};
};

/**
 * @brief Class representing a user in the chat system.
 */
class User : public std::enable_shared_from_this<User> {
private:
  UserData _userData;
  std::shared_ptr<UserChatList> _userChatList; ///< User's chat list.

public:
  /**
   * @brief Constructor for User.
   * @param login User's login.
   * @param userName User's display name.
   * @param passwordHash User's passwordHash.
   */
  //   User(const std::string &login, const std::string &userName,
  //        const std::string &passwordHash);

  User(const UserData &userData);

  /**
   * @brief Default destructor.
   */
  ~User() = default;

  /**
   * @brief Assigns a chat list to the user.
   * @param chats shared pointer to the user's chat list.
   */
  void createChatList();
  void createChatList(const std::shared_ptr<UserChatList> &userChatList);

  std::string getLogin() const;

  std::string getUserName() const;

  std::string getPasswordHash() const;

  std::string getEmail() const;

  std::string getPhone() const;

  bool getIsActive() const;

  std::int64_t getDisabledAt() const;

  std::int64_t getBunUntil() const;

  std::shared_ptr<UserChatList> getUserChatList() const;

  void setPassword(const std::string &passwordHash);

  bool RqFrClientCheckLogin(const std::string &login) const;
};

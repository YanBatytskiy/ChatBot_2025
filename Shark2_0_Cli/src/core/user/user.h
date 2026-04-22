#pragma once

#include <memory>
#include <string>

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

  UserData() = default;

  UserData(const std::string &login, const std::string &name, const std::string &passwordHash, const std::string &email,
           const std::string &phone)
      : _login(login), _passwordHash(passwordHash), _userName(name), _email(email), _phone(phone){};
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

  /**
   * @brief Gets the user's login.
   * @return The user's login string.
   */
  std::string getLogin() const;

  /**
   * @brief Gets the user's display name.
   * @return The user's display name string.
   */
  std::string getUserName() const;

  /**
   * @brief Gets the user's passwordHash.
   * @return The user's passwordHash string.
   */
  std::string getPasswordHash() const;

  /**
   * @brief Gets the user's email.
   * @return The user's Email string.
   */
  std::string getEmail() const;

  /**
   * @brief Gets the user's phone.
   * @return The user's phone string.
   */
  std::string getPhone() const;

  /**
   * @brief Gets the user's chat list.
   * @return shared pointer to the user's chat list.
   */
  std::shared_ptr<UserChatList> getUserChatList() const;

  /**
   * @brief Sets the user's login.
   * @param login The new login string.
   */
  void setLogin(const std::string &login);

  /**
   * @brief Sets the user's display name.
   * @param userName The new display name string.
   */
  void setUserName(const std::string &userName);

  /**
   * @brief Sets the user's passwordHash.
   * @param passwordHash The new passwordHash string.
   */
  void setPassword(const std::string &passwordHash);

  /**
   * @brief Sets the user's email.
   * @param email The new email string.
   */
  void setEmail(const std::string &email);

  /**
   * @brief Sets the user's phone.
   * @param phone The new phone string.
   */
  void setPhone(const std::string &phone);

  /**
   * @brief Checks if the provided passwordHash matches the user's passwordHash.
   * @param passwordHash The passwordHash to check.
   * @return True if the passwordHash matches, false otherwise.
   */
  bool checkPassword(const std::string &passwordHash) const;

  /**
   * @brief Checks if the provided login matches the user's login.
   * @param login The login to check.
   * @return True if the login matches, false otherwise.
   */
  bool RqFrClientCheckLogin(const std::string &login) const;

  /**
   * @brief Prints the user's chat list.
   * @param user shared pointer to the user whose chat list is to be printed.
   */
  void printChatList(const std::shared_ptr<User> &user) const;

  /**
   * @brief Displays the user's data.
   * @details Prints the user's login, name, and other relevant information.
   */
  void showUserData() const;

  /**
   * @brief Displays the user's data for initsysten.
   * @details Prints the user's name, login, and password.
   */
  void showUserDataInit() const;
};

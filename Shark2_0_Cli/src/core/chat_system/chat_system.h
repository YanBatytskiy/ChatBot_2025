#pragma once
#include "chat/chat.h"
#include "dto/dto_struct.h"
#include "user/user.h"
#include <cstddef>
#include <deque>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <cstdint>

#define MESSAGE_LENGTH 4096 // Максимальный размер буфера для данных
#define PORT 50000          // Будем использовать этот номер порта

/**
 * @brief Manages users, chats, and the active user in the chat system.
 */
class ChatSystem {
private:
  std::vector<std::shared_ptr<User>> _users; ///< List of users in the system.
  std::vector<std::shared_ptr<Chat>> _chats; ///< List of chats in the system.
  std::shared_ptr<User> _activeUser;         ///< Current active user.

  std::unordered_map<std::string, std::shared_ptr<User>> _loginUserMap; //!!!
  std::map<std::size_t, std::shared_ptr<Chat>> _chatIdChatMap;          //!!!

  std::set<std::size_t> _freeChatServerIdSet; ///< Set of released chat IDs available for reuse.
  std::size_t _nextChatIdClient = 1;          ///< Next available sequential chat ID.

  bool _isServerStatus = false;

  std::deque<std::pair<PacketDTO, std::string>> _packetReceivedDeque;
  std::deque<std::pair<PacketListDTO, std::string>> _packetForSendDeque;

public:
  /**
   * @brief Default constructor for ChatSystem.
   */
  ChatSystem();

  /**
   * @brief Default destructor.
   */
  ~ChatSystem() = default;

  // getters

  const bool &getIsServerStatus() const;

  std::size_t &getNextChatId();
  const std::size_t &getNextChatId() const;

  std::set<std::size_t> &getFreeChatServerIdSet();
  const std::set<std::size_t> &getFreeChatServerIdSet() const;

  std::shared_ptr<Chat> getChatById(std::size_t chatId) const;

  std::deque<std::pair<PacketDTO, std::string>> &getPacketReceivedDeque();

  std::deque<std::pair<PacketListDTO, std::string>> &getPacketForSendDeque();

  /**
  s thChatMessagesf users.
  * @return Const reference to the vector of users.
  */
  const std::vector<std::shared_ptr<User>> &getUsers() const;

  /**
   * @brief Gets the list of chats.
   * @return Const reference to the vector of chats.
   */
  const std::vector<std::shared_ptr<Chat>> &getChats() const;

  /**
   * @brief Gets the active user.
   * @return Const reference to the active user.
   */
  const std::shared_ptr<User> &getActiveUser() const;

  /**
   * @brief Gets the login user map.
   * @return Const reference to the unordered map.
   */
  const std::unordered_map<std::string, std::shared_ptr<User>> &getLoginUserMap() const;
  //
  //
  //
  // setters

  void setIsServerStatus(const bool &serverStatus);

  void setNextChatIdClient(const std::size_t &nextChatId);

  void setFreeChatServerId(const std::size_t &freeChatId);

  void moveToFreeChatIdSrv(std::size_t chatId);

  void releaseFreeChatIdSrv(std::size_t chatId);

  /**
   * @brief Sets the active user.
   * @param user shared pointer to the user to set as active.
   */
  void setActiveUser(const std::shared_ptr<User> &user);

  /**
   * @brief Adds a user to the system.
   * @param user shared pointer to the user to add.
   */
  void addUserToSystem(std::shared_ptr<User> &user);

  /**
   * @brief Adds a chat to the system.
   * @param chat shared pointer to the chat to add.
   */
  bool addChatToInstance(const std::shared_ptr<Chat> &chat);

  /**
   * @brief Removes a user from the system.
   * @param user shared pointer to the user to remove.
   */
  void eraseUser(const std::shared_ptr<User> &user);

  // utilities
  std::size_t createNewChatId(const std::shared_ptr<Chat> &chat_ptr);

  /**
   * @brief Finds users matching a search string.
   * @param users Vector to store found users.
   * @param textToFind Search string to match against user names or logins.
   */
  std::vector<std::shared_ptr<User>> findUserByTextPart(const std::string &textToFind) const; // поиск пользователя

  /**
   * @brief Displays the list of users.
   * @param showActiveUser True to include the active user in the list.
   * @return The number of users displayed.
   */
  std::size_t showUserList(const bool showActiveUser); // вывод на экрын списка пользователей
  /**
   * @brief Finds a user by login in the chat system.
   * @param userLogin The login to search for.
   * @param chatSystem Reference to the chat system.
   * @return shared pointer to the found user, or nullptr if not found.
   */
  std::shared_ptr<User> findUserByLogin(const std::string &userLogin) const;

  /**
   * @brief Checks if a user with the specified login exists.
   * @param login The login to check.
   * @param chatSystem Reference to the chat system.
   * @return shared pointer to the user if found, or nullptr if not found.
   */
  const std::shared_ptr<User> RqFrClientCheckLoginExists(const std::string &login) const;

  /**
   * @brief Verifies if the provided password matches the user's password.
   * @param userData Structure containing login and password.
   * @param chatSystem Reference to the chat system.
   * @return True if the password is valid, false otherwise.
   */
  bool checkPasswordValidForUser(const std::string &userPassword, const std::string &userLogin);
};
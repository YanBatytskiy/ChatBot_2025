#pragma once
#include "chat/chat.h"
#include "dto/dto_struct.h"
#include "user/user.h"
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#define MESSAGE_LENGTH 4096 // Maximum buffer size for data
#define PORT 50000          // We will use this port number

/**
 * @brief Manages users, chats, and the active user in the chat system.
 */
class ChatSystem {
private:
  std::vector<std::shared_ptr<User>> _users; ///< List of users in the system.
  std::vector<std::shared_ptr<Chat>> _chats; ///< List of chats in the system.
  std::shared_ptr<User> _activeUser;         ///< Current active user.

  // Fast lookup by login during authentication and participant resolution.
  std::unordered_map<std::string, std::shared_ptr<User>> _loginUserMap;
  // Fast lookup by chat id when opening or syncing a chat.
  std::map<std::size_t, std::shared_ptr<Chat>> _chatIdChatMap;

  bool _isServerStatus = false;

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

  std::shared_ptr<Chat> getChatById(std::size_t chatId) const;

  const std::vector<std::shared_ptr<User>> &getUsers() const;

  const std::shared_ptr<User> &getActiveUser() const;

  const std::unordered_map<std::string, std::shared_ptr<User>> &getLoginUserMap() const;
  //
  //
  //
  // setters

  void setIsServerStatus(const bool &serverStatus);

  void setActiveUser(const std::shared_ptr<User> &user);

  void addUserToSystem(std::shared_ptr<User> &user);

  bool addChatToInstance(const std::shared_ptr<Chat> &chat);

  void clear_chat_system();

  // utilities
  std::shared_ptr<User> findUserByLogin(const std::string &userLogin) const;
};

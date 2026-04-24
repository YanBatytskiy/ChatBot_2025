/**
 * @file code_init_system.h
 * @brief Header file for automatic initialization of a test system of chats and messages.
 *
 * Contains the data structure that describes messages and the functions for creating test users,
 * chats and messages in the ChatBot system.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class Chat;
class ServerSession;
class User;

/**
 * @brief Structure to hold initial data for a chat message.
 */
struct InitDataArray {
  std::string _messageText;                       ///< Text content of the message.
  int64_t _timeStamp;                             ///< Timestamp of the message.
  std::shared_ptr<User> _sender;                  ///< Sender of the message.
  std::vector<std::shared_ptr<User>> _recipients; ///< List of message recipients.
  std::size_t _messageId;

  /**
   * @brief Constructor for InitDataArray.
   * @param messageText Text content of the message.
   * @param timeStamp Timestamp of the message.
   * @param sender shared pointer to the sender user.
   * @param _recipients Vector of shared pointers to recipient users.
   */
  InitDataArray(std::string messageText, int64_t timeStamp, std::shared_ptr<User> sender,
                std::vector<std::shared_ptr<User>> _recipients, std::size_t messageId);

  /**
   * @brief Default destructor.
   */
  virtual ~InitDataArray() = default;
};

void changeLastReadIndexForSenderInit(const std::shared_ptr<User> &user, const std::shared_ptr<Chat> &chat);

bool addMessageToChatInit(ServerSession &serverSession, const InitDataArray &initDataArray, std::shared_ptr<Chat> &chat);

void systemInitForTest(ServerSession &serverSession);

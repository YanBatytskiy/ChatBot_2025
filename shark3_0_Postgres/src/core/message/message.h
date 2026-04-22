#pragma once
#include "message_content.h"
#include "user/user.h"
#include <memory>
#include <vector>
#include <cstdint>

/**
 * @brief Interface for messages.
 */
class IMessage { // interface for messages
public:
  /**
   * @brief Default virtual destructor.
   */
  virtual ~IMessage() = default;
};

/**
 * @brief Class representing a message with various content types.
 */
class Message : public IMessage {
private:
  std::vector<std::shared_ptr<IMessageContent>> _content; ///< Vector of message content.
  std::weak_ptr<User> _sender;                            ///< Sender of the message.
  int64_t _time_stamp;                                    ///< Timestamp of the message (to be implemented).
  std::size_t _messageId;

public:
  Message();
  Message(const std::vector<std::shared_ptr<IMessageContent>> &content, const std::weak_ptr<User> &sender,
          const int64_t &timeStamp, std::size_t messageId);

  /**
   * @brief Default destructor.
   */
  ~Message() override = default; // destructor

  // getters
  const std::size_t &getMessageId() const;

  std::weak_ptr<User> getSender() const;

  const int64_t &getTimeStamp() const;

  const std::vector<std::shared_ptr<IMessageContent>> &getContent() const;

  // setters
  void setMessageId(std::size_t messageId);

  void addContent(const std::shared_ptr<IMessageContent> &content);

  // utilities
  void printMessage(const std::shared_ptr<User> &currentUser);
};

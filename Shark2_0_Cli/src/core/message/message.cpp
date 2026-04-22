#include "message.h"
#include "message_content.h"
#include "message_content_struct.h"
#include "system/date_time_utils.h"
#include <iostream>
#include <memory>
#include <cstdint>

Message::Message(const std::vector<std::shared_ptr<IMessageContent>> &content, const std::weak_ptr<User> &sender,
                 const int64_t &timeStamp, std::size_t messageId)
    : _content(content), _sender(sender), _time_stamp(timeStamp), _messageId(messageId) {}

// getters
const std::size_t &Message::getMessageId() const { return _messageId; }

std::weak_ptr<User> Message::getSender() const { return _sender; }

const int64_t &Message::getTimeStamp() const { return _time_stamp; }

const std::vector<std::shared_ptr<IMessageContent>> &Message::getContent() const { return _content; }

// setters
void Message::setMessageId(std::size_t messageId) { _messageId = messageId; }

void Message::addContent(const std::shared_ptr<IMessageContent> &content) { _content.push_back(content); }

/**
 * @brief Prints the message for a specific user.
 * @param currentUser shared pointer to the user viewing the message.
 * @details Displays the message with formatting based on whether it is incoming or outgoing, including sender details
 * and content.
 * @note Contains a typo in the parameter name (¤tUser).
 */
void Message::printMessage(const std::shared_ptr<User> &currentUser) {

  // определили отправителя
  auto sender_ptr = _sender.lock();

  // определили направление сообщения
  bool messageDirection;

  if (sender_ptr == nullptr) {
    messageDirection = false;
  } else
    messageDirection = (sender_ptr == currentUser);

  if (!messageDirection) { // income Message

    std::cout << "\033[32m"; // green

    if (sender_ptr != nullptr) {
      std::cout << "     <- Входящее от Логин/Имя:    " << sender_ptr->getLogin() << "/" << sender_ptr->getUserName()
                << "    " << formatTimeStampToString(_time_stamp, true) << ", messageId: " << _messageId << std::endl;
    } else {
      std::cout << "     <- Входящее от Логин/Имя:    " << "Пользователь удален.    "
                << formatTimeStampToString(_time_stamp, true) << ", messageId: " << _messageId << std::endl;
    }
  } else {

    std::cout << "\033[37m"; // white
    std::cout << "-> Исходящее от тебя: " << currentUser->getUserName() << "    "
              << formatTimeStampToString(_time_stamp, true) << " messageId: " << _messageId << std::endl;
  }

  for (const auto &content : _content) {
    if (auto textContent = std::dynamic_pointer_cast<MessageContent<TextContent>>(content)) {
      std::cout << textContent->getMessageContent()._text << std::endl;
    } else if (auto imageContent = std::dynamic_pointer_cast<MessageContent<ImageContent>>(content)) {
      std::cout << imageContent->getMessageContent()._image << std::endl;
    }
  }

  std::cout << "\033[0m";
}

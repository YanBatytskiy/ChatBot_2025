#include "message.h"
#include "message_content.h"
// #include "message_content_struct.h"
// #include "system/date_time_utils.h"
// #include <iostream>
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

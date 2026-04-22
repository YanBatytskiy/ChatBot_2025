#pragma once

#include "message/message.h"
#include "user/user.h"
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>
/**
 * @struct Participant
 * @brief Represents a participant in a chat.
 */
struct Participant {
  std::weak_ptr<User> _user; ///< Weak reference to the user.
                             ///< participant.
  bool _deletedFromChat;     ///< Indicates if the participant was removed from the
                             ///< chat.
};

/**
 * @class Chat
 * @brief Manages a chat system with participants and messages.
 */
class Chat {
private:
  std::size_t _chatId;
  std::vector<Participant> _participants; ///< List of chat participants.

  // int64_t — это timeStamp
  // multimap автоматически сортирует по ключу (времени)
  // допускает несколько сообщений с одинаковым временем
  std::multimap<int64_t, std::shared_ptr<Message>> _messages;

  // global list of messageId
  std::set<std::size_t> _messageIdMap;

  // userLogin - MessageId
  std::unordered_map<std::string, std::size_t> _lastReadMessageMap;

  std::unordered_map<std::size_t, int64_t> _messageIdToTimeStamp; //!!!

  // Ключ — login
  //  Значение — множество messageId, которые пользователь удалил
  std::unordered_map<std::string, std::unordered_set<std::size_t>> _deletedMessagesMap; //!!!

public:
  Chat() = default;
  Chat(const Chat &) = delete;
  Chat &operator=(const Chat &) = delete;

  /**
   * @brief Default virtual destructor.
   */
  virtual ~Chat() = default;

  // getters

  std::set<std::size_t> &getMessageIdMap();
  const std::set<std::size_t> &getMessageIdMap() const;

  const std::size_t &getChatId() const;

  const std::unordered_map<std::size_t, int64_t> &getMessageIdToTimeStamp() const;

  const std::multimap<int64_t, std::shared_ptr<Message>> &getMessages() const;
  std::multimap<int64_t, std::shared_ptr<Message>> &getMessages();

  std::unordered_map<std::string, std::unordered_set<std::size_t>> &getDeletedMessagesMap();
  const std::unordered_map<std::string, std::unordered_set<std::size_t>> &getDeletedMessagesMap() const;

  const std::vector<Participant> &getParticipants() const;

  std::size_t getLastReadMessageId(const std::shared_ptr<User> &user) const;

  bool getUserDeletedFromChat(const std::shared_ptr<User> &user) const;

  // setters

  void setMessageIdMap(const std::size_t &messageId);

  void setChatId(const std::size_t &chatId);

  void setMessageIdToTimeStamp(const std::size_t &messageId, const int64_t &timeStamp);

  void addParticipant(const std::shared_ptr<User> &user, std::size_t lastReadMessageIndex, bool deletedFromChat);

  void addMessageToChat(const std::shared_ptr<Message> &message, const std::shared_ptr<User> &sender,
                        const bool &isServerSession);

  void setLastReadMessageId(const std::shared_ptr<User> &user, std::size_t newLastReadMessageId);

  bool setDeletedMessageMap(const std::string &userLogin, const std::size_t &deletedMessageId);

  std::size_t getUnreadMessageCount(const std::shared_ptr<User> &user_ptr);

  };

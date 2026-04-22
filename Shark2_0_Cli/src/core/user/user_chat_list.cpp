#include "user/user_chat_list.h"
#include "user/user.h"
#include <algorithm>

/**
 * @brief Constructor for the user's chat list.
 * @param owner shared pointer to the user who owns the chat list.
 */
UserChatList::UserChatList(const std::shared_ptr<User> &owner) : _owner(owner) {}

// getters
/**
 * @brief Gets the owner of the chat list.
 * @return Weak pointer to the user who owns the chat list.
 */
std::weak_ptr<User> UserChatList::getOwner() const { return _owner; }

/**
 * @brief Gets the list of chats for the user.
 * @return Vector of weak pointers to chats.
 */
std::vector<std::weak_ptr<Chat>> UserChatList::getChatFromList() const { return _chatList; }

// setters

/**
 * @brief Adds a chat to the user's chat list.
 * @param chat Weak pointer to the chat to add.
 */
void UserChatList::addChatToChatList(const std::weak_ptr<Chat> &chat) {

auto new_ptr = chat.lock();
  if (!new_ptr)
    return;

  for (const auto &chat_ptr : _chatList)
    if (chat_ptr.lock() == new_ptr)
      return;

  _chatList.push_back(chat);}

void UserChatList::deleteChatFromList(const std::weak_ptr<Chat> &chat) {

  auto it = std::find_if(_chatList.begin(), _chatList.end(), [&chat](const std::weak_ptr<Chat> &item) {
    return !item.owner_before(chat) && !chat.owner_before(item);
  });

  if (it != _chatList.end()) {
    _chatList.erase(it);
  }
}

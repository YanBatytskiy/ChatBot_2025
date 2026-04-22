#include "client/processors/client_session_dto_setter.h"

#include <QString>
#include <memory>
#include <vector>

#include "chat/chat.h"
#include "chat_system/chat_system.h"
#include "client/client_session.h"
#include "core/system/system_function.h"
#include "dto_struct.h"
#include "message/message.h"
#include "system/date_time_utils.h"
#include "system/system_function.h"
#include "user/user.h"
#include "user/user_chat_list.h"

ClientSessionDtoSetter::ClientSessionDtoSetter(ClientSession &session)
    : session_(session) {}

void ClientSessionDtoSetter::setActiveUserDTOFromSrvProcessing(
    const UserDTO &user_dto) const {
  auto user_ptr = std::make_shared<User>(
      UserData(user_dto.login, user_dto.userName, user_dto.passwordhash,
               user_dto.email, user_dto.phone, user_dto.disable_reason,
               user_dto.is_active, user_dto.disabled_at, user_dto.ban_until));

  auto &instance = session_.getInstanceCl();

  instance.addUserToSystem(user_ptr);
  instance.setActiveUser(user_ptr);
  user_ptr->createChatList(std::make_shared<UserChatList>(user_ptr));
}

void ClientSessionDtoSetter::setOneMessageDTOProcessing(
    const MessageDTO &message_dto, const std::shared_ptr<Chat> &chat) const {
  auto sender =
      session_.getInstanceCl().findUserByLogin(message_dto.senderLogin);

  auto message = createOneMessage(message_dto.messageContent[0].payload, sender,
                                  message_dto.timeStamp, message_dto.messageId);

  chat->addMessageToChat(std::make_shared<Message>(message), sender, false);
}

bool ClientSessionDtoSetter::setOneChatMessageDTOProcessing(
    const MessageChatDTO &message_chat_dto) const {
  const auto &active_user = session_.getInstanceCl().getActiveUser();
  if (!active_user) {
    return false;
  }

  const auto &chat_list = active_user->getUserChatList();
  if (!chat_list) {
    return false;
  }

  auto chats = chat_list->getChatFromList();

  std::shared_ptr<Chat> chat_ptr;
  for (const auto &chat : chats) {
    chat_ptr = chat.lock();

    if (chat_ptr && chat_ptr->getChatId() == message_chat_dto.chatId) {
      for (const auto &message : message_chat_dto.messageDTO) {
        setOneMessageDTOProcessing(message, chat_ptr);
      }
    }
  }
  return true;
}

void ClientSessionDtoSetter::setOneChatDTOFromSrvProcessing(
    const ChatDTO &chat_dto) {
  auto chat_ptr = std::make_shared<Chat>();
  auto active_user = session_.getActiveUserCl();
  if (!active_user || !active_user->getUserChatList()) {
    return;
  }

  chat_ptr->setChatId(chat_dto.chatId);

  std::vector<std::string> participants;
  participants.reserve(chat_dto.participants.size());
  for (const auto &participant : chat_dto.participants) {
    participants.push_back(participant.login);
  }

  session_.checkAndAddParticipantToSystemCl(participants);

  for (const auto &participant : chat_dto.participants) {
    for (const auto &deleted_message_id : participant.deletedMessageIds) {
      chat_ptr->setDeletedMessageMap(participant.login, deleted_message_id);
    }

    auto user_ptr = session_.getInstanceCl().findUserByLogin(participant.login);

    if (user_ptr) {
      chat_ptr->setLastReadMessageId(user_ptr, participant.lastReadMessage);
      chat_ptr->addParticipant(user_ptr, participant.lastReadMessage,
                               participant.deletedFromChat);
    }
  }

  session_.getInstanceCl().addChatToInstance(chat_ptr);
}

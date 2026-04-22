#include "client/processors/client_session_dto_builder.h"

#include <memory>
#include <optional>
#include <vector>

#include "chat/chat.h"
#include "client/client_session.h"
#include "dto_struct.h"
#include "exceptions_qt/exception_valid.h"
#include "message/message.h"
#include "message/message_content_struct.h"
#include "user/user.h"

ClientSessionDtoBuilder::ClientSessionDtoBuilder(ClientSession &session)
    : session_(session) {}

MessageDTO ClientSessionDtoBuilder::fillOneMessageDTOFromProcessing(
    const std::shared_ptr<Message> &message, std::size_t chat_id) {
  MessageDTO message_dto;
  auto user_ptr = message->getSender().lock();

  message_dto.chatId = chat_id;
  message_dto.messageId = message->getMessageId();
  message_dto.senderLogin = user_ptr ? user_ptr->getLogin() : "";

  MessageContentDTO content_dto;
  content_dto.messageContentType = MessageContentType::Text;
  if (message->getContent().empty()) {
    throw exc_qt::UnknownException("Пустой контент сообщения");
  }
  auto content_element = message->getContent().front();

  auto content_text_ptr =
      std::dynamic_pointer_cast<MessageContent<TextContent>>(content_element);

  if (content_text_ptr) {
    auto content_text = content_text_ptr->getMessageContent();
    content_dto.payload = content_text._text;
  }
  message_dto.messageContent.push_back(content_dto);

  message_dto.timeStamp = message->getTimeStamp();

  return message_dto;
}

std::optional<ChatDTO> ClientSessionDtoBuilder::fillChatDTOProcessing(
    const std::shared_ptr<Chat> &chat_ptr) {
  ChatDTO chat_dto;

  chat_dto.chatId = chat_ptr->getChatId();

  auto &instance = session_.getInstanceCl();
  if (!instance.getActiveUser()) {
    return std::nullopt;
  }
  chat_dto.senderLogin = instance.getActiveUser()->getLogin();

  auto participants = chat_ptr->getParticipants();

  if (participants.empty()) {
    return std::nullopt;
  }

  for (const auto &participant : participants) {
    const auto user_ptr = participant._user.lock();

    ParticipantsDTO participants_dto;

    if (user_ptr) {
      participants_dto.login = user_ptr->getLogin();
      participants_dto.lastReadMessage =
          chat_ptr->getLastReadMessageId(user_ptr);

      participants_dto.deletedMessageIds.clear();
      const auto &deleted_messages_map = chat_ptr->getDeletedMessagesMap();
      const auto deleted_messages_it =
          deleted_messages_map.find(participants_dto.login);

      if (deleted_messages_it != deleted_messages_map.end()) {
        for (const auto &deleted_message_id : deleted_messages_it->second) {
          participants_dto.deletedMessageIds.push_back(deleted_message_id);
        }
      }

      participants_dto.deletedFromChat = false;

      chat_dto.participants.push_back(participants_dto);
    }
  }

  if (chat_dto.participants.empty()) {
    return std::nullopt;
  }
  return chat_dto;
}

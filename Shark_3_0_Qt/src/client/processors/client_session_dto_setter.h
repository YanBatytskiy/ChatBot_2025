#pragma once

#include <memory>

class ClientSession;
class Chat;
struct ChatDTO;
struct MessageChatDTO;
struct MessageDTO;
struct UserDTO;

class ClientSessionDtoSetter {
public:
  explicit ClientSessionDtoSetter(ClientSession &session);

  void setActiveUserDTOFromSrvProcessing(const UserDTO &user_dto) const;
  void setOneMessageDTOProcessing(const MessageDTO &message_dto,
                                  const std::shared_ptr<Chat> &chat) const;
  bool
  setOneChatMessageDTOProcessing(const MessageChatDTO &message_chat_dto) const;
  void setOneChatDTOFromSrvProcessing(const ChatDTO &chat_dto);

private:
  ClientSession &session_;
};

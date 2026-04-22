#pragma once

#include <cstddef>
#include <memory>

#include "chat/chat.h"
#include "client/processors/client_session_dto_builder.h"
#include "dto_struct.h"
#include "message/message.h"
#include "user/user.h"

class ClientSession;
class ClientCore;

class ClientSessionCreateObjects {
 public:
  ClientSessionCreateObjects(ClientSession &session, ClientCore &core,
                             ClientSessionDtoBuilder &dto_builder);

  bool createUserProcessing(const UserDTO &user_dto);
  bool createNewChatProcessing(std::shared_ptr<Chat> &chat, ChatDTO &chat_dto,
                               MessageChatDTO &message_chat_dto);
  std::size_t createMessageProcessing(const Message &message,
                                      std::shared_ptr<Chat> &chat_ptr,
                                      const std::shared_ptr<User> &user);

 private:
  ClientSession &session_;
  ClientCore &core_;
  ClientSessionDtoBuilder &dto_builder_;
};

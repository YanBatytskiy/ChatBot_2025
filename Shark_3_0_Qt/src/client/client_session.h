#pragma once

#include <QObject>
#include <atomic>
#include <optional>
#include <thread>
#include <vector>

#include "chat/chat.h"
#include "chat_system/chat_system.h"
#include "client/core/client_core.h"
#include "client/processors/client_session_create_objects.h"
#include "client/processors/client_session_dto_builder.h"
#include "client/processors/client_session_dto_setter.h"
#include "client/processors/client_session_modify_objects.h"
#include "client/tcp_transport/session_types.h"
#include "dto_struct.h"
#include "user/user.h"

class ClientSession : public QObject {
  Q_OBJECT

private:
  void connectionMonitorLoopCl();
  static bool socketAliveCl(int fd);

  ChatSystem &_instance;
  ClientCore _core;
  ClientSessionDtoSetter _dtoSetter;
  ClientSessionDtoBuilder _dtoBuilder;
  ClientSessionCreateObjects _createObjects;
  ClientSessionModifyObjects _modifyObjects;
  std::atomic_bool connection_thread_running_{false};
  std::thread connection_thread_;

public:
  explicit ClientSession(ChatSystem &chat_system, QObject *parent = nullptr);
  ClientSession(const ClientSession &) = delete;
  ClientSession &operator=(const ClientSession &) = delete;
  ClientSession(ClientSession &&) = delete;
  ClientSession &operator=(ClientSession &&) = delete;

  // constructors
  ~ClientSession();

  // getters
  ChatSystem &getInstanceCl();
  const std::shared_ptr<User> getActiveUserCl() const;
//   int getSocketFdCl() const;
  bool getIsServerOnlineCl() const noexcept;
  ServerConnectionConfig &getserverConnectionConfigCl();
  const ServerConnectionConfig &getserverConnectionConfigCl() const;
  ServerConnectionMode &getserverConnectionModeCl();
  const ServerConnectionMode &getserverConnectionModeCl() const;

  std::optional<
      std::multimap<std::int64_t, ChatDTO, std::greater<std::int64_t>>>
  getChatListCl();

  // setters
//   void setSocketFdCl(int socket_fd);

  // threads
  void startConnectionThreadCl();
  void stopConnectionThreadCl();

  // utilities
  void clearChatSystemCl();
  bool reInitilizeBaseCl();
  bool registerClientToSystemCl(const std::string &login);

  bool CreateAndSendNewChatCl(std::shared_ptr<Chat> &chat_ptr,
                              std::vector<UserDTO> &participants,
                              Message &new_message);
  bool createUserCl(const UserDTO &user_dto);
  bool createNewChatCl(std::shared_ptr<Chat> &chat, ChatDTO &chat_dto,
                       MessageChatDTO &message_chat_dto);
  std::size_t createMessageCl(const Message &message,
                              std::shared_ptr<Chat> &chat,
                              const std::shared_ptr<User> &user);

  bool changeUserDataCl(const UserDTO &user_dto);
  bool changeUserPasswordCl(UserDTO user_dto);
  bool blockUnblockUserCl(const std::string &login, bool is_blocked,
                          const std::string &disable_reason);
  bool bunUnbunUserCl(const std::string &login, bool is_banned,
                      std::int64_t banned_to);
  bool sendLastReadMessageFromClientCl(const std::shared_ptr<Chat> &chat_ptr,
                                       std::size_t message_id);

  bool checkUserLoginCl(const std::string &user_login);
  bool checkUserPasswordCl(const std::string &user_login,
                           const std::string &password);
  bool checkAndAddParticipantToSystemCl(
      const std::vector<std::string> &participants);

  const std::vector<UserDTO>
  findUserByTextPartOnServerCl(const std::string &text_to_find);

signals:
  void serverStatusChanged(bool online, ServerConnectionMode mode);
};

#pragma once
#include "dto/dto_struct.h"
#include "packet_parser/packet_parser.h"
#include "processors/user_account_update_processor.h"
#include "processors/user_ban_block_processor.h"
#include "processors/user_database_init_processor.h"
#include "processors/user_object_creation_processor.h"
#include "processors/user_registration_processor.h"
#include "processors/user_data_query_processor.h"
#include "session_transport/session_transport.h"
#include "sql_queries/chat_sql_reader.h"
#include "sql_queries/chat_sql_writer.h"
#include "sql_queries/database_sql_manager.h"
#include "sql_queries/message_sql_writer.h"
#include "sql_queries/user_sql_reader.h"
#include "sql_queries/user_sql_writer.h"
#include "sql_commands/sql_commands.h"
#include <cstdint>
#include <libpq-fe.h>
#include <optional>
#include <vector>

#define MESSAGE_LENGTH 4096 // Максимальный размер буфера для данных

struct ServerConnectionConfig {
  std::string addressLocalHost = "127.0.0.1";
  std::string addressLocalNetwork = "";
  std::string addressInternet = "https://yan2201moscowoct.ddns.net";
  std::uint16_t port = 50000;
  bool found = false;
};

class ServerSession {
private:
  PacketParser packet_parser_;
  ServerConnectionConfig _serverConnectionConfig;
  PGconn *_pqConnection;
  SQLRequests sql_requests_;
  UserSqlReader user_sql_reader_;
  UserSqlWriter user_sql_writer_;
  ChatSqlReader chat_sql_reader_;
  ChatSqlWriter chat_sql_writer_;
  MessageSqlWriter message_sql_writer_;
  DatabaseSqlManager database_sql_manager_;
  UserBanBlockProcessor user_ban_block_processor_;
  UserAccountUpdateProcessor user_account_update_processor_;
  UserDatabaseInitProcessor user_database_init_processor_;
  UserRegistrationProcessor user_registration_processor_;
  UserObjectCreationProcessor user_object_creation_processor_;
  UserDataQueryProcessor user_data_query_processor_;
  

public:
  // constructors
  explicit ServerSession(SQLRequests sql_requests);

  // getters
  PGconn *getPGConnection();
  const PGconn *getPGConnection() const;

  ServerConnectionConfig &getServerConnectionConfig();

  void setPgConnection(PGconn *connection);

  void ProcessIncoming(SessionTransport &transport);

  bool sendPacketListDTO(SessionTransport &transport,
                         PacketListDTO &packetListForSend, int connection);
  //
  //
  // Request processing

  bool routingRequestsFromClient(SessionTransport &transport,
                                 PacketListDTO &packetListReceived,
                                 const RequestType &requestType,
                                 int connection);

  // transport sending
  std::optional<UserDTO> FillForSendUserDTOFromSrvSQL(const std::string &userLogin, bool loginUser);

  std::optional<std::vector<UserDTO>> FillForSendSeveralUsersDTOFromSrvSQL(const std::vector<std::string> logins);

  std::optional<ChatDTO> FillForSendOneChatDTOFromSrvSQL(const std::string &chat_id, const std::string &login);

  std::optional<std::vector<ChatDTO>> FillForSendAllChatDTOFromSrvSQL(const std::string &login);

  // получаем сообщения пользователя конкретного чата
  std::optional<MessageChatDTO> fillForSendChatMessageDTOFromSrvSQL(const std::string &chat_id);

  // получаем все сообщения пользователя
  std::optional<std::vector<MessageChatDTO>> fillForSendAllMessageDTOFromSrvSQL(const std::string login);

  // utilities

  void runUDPServerDiscovery(std::uint16_t listenPort);

  std::optional<PacketListDTO> registerOnDeviceDataSrvSQL(const std::string login);
};

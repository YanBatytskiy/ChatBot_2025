#pragma once
#include <libpq-fe.h>
#include "dto/dto_struct.h"
#include <cstdint>
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
  int _connection;
  ServerConnectionConfig _serverConnectionConfig;
  PGconn *_pqConnection;

public:
  // constructors
  ServerSession() = default;

  // getters
  PGconn *getPGConnection();
  const PGconn *getPGConnection() const;

  ServerConnectionConfig &getServerConnectionConfig();

  int &getConnection();
  const int &getConnection() const;
  //
  //
  // setters
  void setPgConnection(PGconn *connection);

  void setConnection(const std::uint8_t &connection);
  //
  //
  // transport

  bool isConnected() const;

  void runServer(int socketFd);

  void listeningClients();

  bool sendPacketListDTO(PacketListDTO &packetListForSend, int connection);
  //
  //
  // Request processing

  bool routingRequestsFromClient(PacketListDTO &packetListReceived, const RequestType &requestType, int connection);

  bool processingCheckAndRegistryUser(PacketListDTO &packetListReceived, const RequestType &requestType,
                                      int connection);

  bool processingCreateObjects(PacketListDTO &packetListReceived, const RequestType &requestType, int connection);

  bool processingGetUserData(PacketListDTO &packetListReceived, const RequestType &requestType, int connection);

  //
  //
  //
  bool processingReceivedtWithoutUser(std::vector<PacketDTO> &packetListReceived, int connection);

  // utilities
  bool checkUserLoginSrvSQL(const std::string &login);

  bool checkUserPasswordSrvSql(const UserLoginPasswordDTO &userLoginPasswordDTO);

  std::string getUserPasswordSrvSql(const UserLoginPasswordDTO &userLoginPasswordDTO);

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
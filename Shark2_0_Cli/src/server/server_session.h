#pragma once
#include "chat/chat.h"
#include "chat_system/chat_system.h"
#include "dto/dto_struct.h"
#include <cstdint>
#include <memory>
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
  ChatSystem &_instance; // link to server
  std::map<std::string, int> _loginToConnectionMap;
  int _connection;
  ServerConnectionConfig _serverConnectionConfig;

public:
  // constructors
  ServerSession(ChatSystem &server);

  // getters

  ServerConnectionConfig &getServerConnectionConfig();

  int &getConnection();
  const int &getConnection() const;

  const std::shared_ptr<User> getActiveUserSrv() const;

  ChatSystem &getInstance();

  //
  //
  // setters
  void setActiveUserSrv(const std::shared_ptr<User> &user);

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

  bool routingRequestsFromClient(PacketListDTO &packetListReceived, const RequestType& requestType, int connection);

  bool processingCheckAndRegistryUser(PacketListDTO &packetListReceived, const RequestType& requestType, int connection);

  bool processingCreateObjects(PacketListDTO &packetListReceived, const RequestType& requestType, int connection);

  bool processingGetIndexes(PacketListDTO &packetListReceived, const RequestType& requestType, int connection);

  //
  //
  //
  bool processingReceivedtWithoutUser(std::vector<PacketDTO> &packetListReceived, int connection);

  bool processingReceivedQueue(const std::string &userLogin);

  bool processingSendQueue();
  //
  //
  //

  // utilities
  const std::vector<UserDTO> findUserByTextPartFromSrv(const std::string &textToFind) const;

  bool checkUserLoginSrv(const UserLoginDTO &userLoginDTO) const;

  bool checkUserPasswordSrv(const UserLoginPasswordDTO &userLoginPasswordDTO) const;

  // transport sending
  UserDTO FillForSendUserDTOFromSrv(const std::string &userLogin, bool loginUser) const;

  std::optional<ChatDTO> FillForSendOneChatDTOFromSrv(const std::shared_ptr<Chat> &chat,
                                                      const std::shared_ptr<User> &user);

  std::optional<std::vector<ChatDTO>> FillForSendAllChatDTOFromSrv(const std::shared_ptr<User> &user);

  // получаем одно конкретное сообщение пользователя
  std::optional<MessageDTO> FillForSendOneMessageDTOFromSrv(const std::shared_ptr<Message> &message,
                                                            const std::size_t &chatId);

  // получаем сообщения пользователя конкретного чата
  std::optional<MessageChatDTO> fillForSendChatMessageDTOFromSrv(const std::shared_ptr<Chat> &chat);

  // получаем все сообщения пользователя
  std::optional<std::vector<MessageChatDTO>> fillForSendAllMessageDTOFromSrv(const std::shared_ptr<User> &user);

  // utilities

  void runUDPServerDiscovery(std::uint16_t listenPort);

  bool createUserSrv(const UserDTO &userDTO) const;

  std::size_t createNewChatSrv(std::shared_ptr<Chat> &chat, ChatDTO chatDTO) const;

  std::size_t createNewMessageSrv(const MessageDTO &messageDTO) const;

  bool createNewMessageChatSrv(std::shared_ptr<Chat> &chat, MessageChatDTO &messageChatDTO);

  std::optional<PacketListDTO> registerOnDeviceDataSrv(const std::shared_ptr<User> &user_ptr);
};
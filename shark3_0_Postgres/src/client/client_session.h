#pragma once
#include "chat_system/chat_system.h"
#include "dto_struct.h"
#include "message/message.h"
#include <memory>
#include <optional>
#include <cstdint>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#endif


enum class ServerConnectionMode {
  Localhost,    // на этом же компьютере
  LocalNetwork, // внутри LAN (UDP discovery)
  Internet,     // через DDNS / внешний IP
  Offline       //
};

struct ServerConnectionConfig {
  std::string addressLocalHost = "127.0.0.1";
  std::string addressLocalNetwork = "";
  std::string addressInternet = "https://yan2201moscowoct.ddns.net";
  std::uint16_t port = 50000;
  bool found = false;
};

class ClientSession {
private:
  ChatSystem &_instance; // link to server
  int _socketFd;
  ServerConnectionConfig _serverConnectionConfig;
  ServerConnectionMode _serverConnectionMode;

public:
  // constructors
  ClientSession(ChatSystem &client);

  // getters
  ServerConnectionConfig &getserverConnectionConfigCl();

  const ServerConnectionConfig &getserverConnectionConfigCl() const;

  ServerConnectionMode &getserverConnectionModeCl();

  const ServerConnectionMode &getserverConnectionModeCl() const;

  const std::shared_ptr<User> getActiveUserCl() const;

  ChatSystem &getInstance();

  int &getSocketFd();
  const int &getSocketFd() const;

  // setters

  void setActiveUserCl(const std::shared_ptr<User> &user);

  void setSocketFd(const int &socketFd);

  // checking and finding

  // поиск и проверки пользователя
  const std::vector<UserDTO> findUserByTextPartOnServerCl(const std::string &textToFind);

  bool checkUserLoginCl(const std::string &userLogin);

  bool checkUserPasswordCl(const std::string &userLogin, const std::string &passwordHash);

  // transport

  void reidentifyClientAfterConnection();

  bool findServerAddress(ServerConnectionConfig &serverConnectionConfig, ServerConnectionMode &serverConnectionMode);

  int createConnection(ServerConnectionConfig &serverConnectionConfig, ServerConnectionMode &serverConnectionMode);

  bool discoverServerOnLAN(ServerConnectionConfig &serverConnectionConfig);

  bool checkResponceServer();

  PacketListDTO getDatafromServer(const std::vector<std::uint8_t> &packetListSend);

  PacketListDTO processingRequestToServer(std::vector<PacketDTO> &packetDTOVector, const RequestType &requestType);

  //
  //
  // utilities

  void resetSessionData();

  const std::size_t createNewMessageIdFromCl() const;

  bool registerClientToSystemCl(const std::string &login);

  bool createUserCl( std::shared_ptr<User> &user);

  bool createNewChatCl(std::shared_ptr<Chat> &chat);

  bool createMessageCl(const Message &message, std::shared_ptr<Chat> &chat, const std::shared_ptr<User> &user);

  // отправить на сервер lastReadMessage
  bool sendLastReadMessageFromClient(const std::shared_ptr<Chat> &chat_ptr, std::size_t messageId);
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  // transport setters

  // установка пользователя
  void setActiveUserDTOFromSrv(const UserDTO &userDTO) const;

  void setUserDTOFromSrv(const UserDTO &userDTO) const;

  //   установка одного сообщения
  void setOneMessageDTO(const MessageDTO &messageDTO, const std::shared_ptr<Chat> &chat) const;

  // установка сообщений одного чата
  bool setOneChatMessageDTO(const MessageChatDTO &messageChatDTO) const;

  bool checkAndAddParticipantToSystem(const std::vector<std::string> &participants);

  // установка чат лист пльзователя

  void setOneChatDTOFromSrv(const ChatDTO &chatDTO);
  MessageDTO fillOneMessageDTOFromCl(const std::shared_ptr<Message> &message, std::size_t chatId);

  // заполнение на отправку пакета Chat
  std::optional<ChatDTO> FillForSendOneChatDTOFromClient(const std::shared_ptr<Chat> &chat);

  MessageDTO FillForSendOneMessageDTOFromClient(const std::shared_ptr<Message> &message, const std::size_t &chatId);

  // передаем сообщения пользователя конкретного чата
  MessageChatDTO fillChatMessageDTOFromClient(const std::shared_ptr<Chat> &chat);
};
#pragma once
#include "core/chat_system/chat_system.h"
#include "core/message/message.h"
#include "dto/dto_struct.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

enum class ServerConnectionMode {
  Localhost,    // on the same computer
  LocalNetwork, // inside the LAN (UDP discovery)
  Internet,     // via DDNS / external IP
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

  // User search and validation
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

  std::size_t createNewMessageIdFromCl() const;

  bool registerClientToSystemCl(const std::string &login);

  bool createUserCl(std::shared_ptr<User> &user);

  bool createNewChatCl(std::shared_ptr<Chat> &chat);

  bool createMessageCl(const Message &message, std::shared_ptr<Chat> &chat, const std::shared_ptr<User> &user);

  // Send lastReadMessage to the server
  bool sendLastReadMessageFromClient(const std::shared_ptr<Chat> &chat_ptr, std::size_t messageId);
  // Transport setters and DTO-to-domain synchronization helpers.

  // Set the user
  void setActiveUserDTOFromSrv(const UserDTO &userDTO) const;

  void setUserDTOFromSrv(const UserDTO &userDTO) const;

  //   Set a single message
  void setOneMessageDTO(const MessageDTO &messageDTO, const std::shared_ptr<Chat> &chat) const;

  // Set the messages of a single chat
  bool setOneChatMessageDTO(const MessageChatDTO &messageChatDTO) const;

  bool checkAndAddParticipantToSystem(const std::vector<std::string> &participants);

  // Set the user's chat list

  void setOneChatDTOFromSrv(const ChatDTO &chatDTO);
  MessageDTO fillOneMessageDTOFromCl(const std::shared_ptr<Message> &message, std::size_t chatId);

  // Populate a Chat packet for sending
  std::optional<ChatDTO> FillForSendOneChatDTOFromClient(const std::shared_ptr<Chat> &chat);

  MessageDTO FillForSendOneMessageDTOFromClient(const std::shared_ptr<Message> &message, const std::size_t &chatId);

  // Forward the user's messages for a specific chat
  MessageChatDTO fillChatMessageDTOFromClient(const std::shared_ptr<Chat> &chat);
};

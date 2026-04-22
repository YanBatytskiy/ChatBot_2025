#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

enum class RequestType {

  UserHeder = 0,
  // Requests and Data to Server

  // check and registry User
  RqFrClientCheckLogin = 1,
  RqFrClientCheckLogPassword = 2,
  RqFrClientRegisterUser = 3,
  RqFrClientFindUserByPart = 4,
  RqFrClientFindUserByLogin = 5,

  // create objects
  RqFrClientCreateUser = 6,
  RqFrClientCreateChat = 7,
  RqFrClientCreateMessage = 8,

  // work indexes
  RqFrClientGetUsersData = 9,
  RqFrClientSetLastReadMessage = 10,

  // transport
  RqFrClientUserConnectMake = 11,
  RqFrClientSynchroUser = 12,

  // Requests and Data from Server

  // create objects
  RqFrSrvCreateUser = 13,
  RqFrSrvCreateChat = 14,
  RqFrSrvCreateMessage = 15,

  // transport
  RqFrSrvSynchroUser = 16,
};

enum class StructDTOClassType {
  messageDTO,
  messageChatDTO,
  chatDTO,
  userDTO,
  userLoginDTO,
  userLoginPasswordDTO,
  responceDTO,
};

enum class RequestDirection { ClientToSrv = 1, SrvToClient = 2 };

class IStructDTO {
public:
  virtual ~IStructDTO() = default;
};

template <typename T> class StructDTOClass : public IStructDTO {
private:
  T _structDTO;

public:
  explicit StructDTOClass(const T &structDTO) : _structDTO(structDTO){};

  // getters
  T &getStructDTOClass() { return _structDTO; };
  const T &getStructDTOClass() const { return _structDTO; };

  // setters
  //   void setStructDTOClass(const T &structDTO) {};
};

struct PacketDTO {
  RequestType requestType;
  StructDTOClassType structDTOClassType;
  RequestDirection reqDirection;
  std::shared_ptr<IStructDTO> structDTOPtr;
};

struct PacketListDTO {
  std::vector<PacketDTO> packets;
};

enum class MessageContentType { Text, Image, File };

struct MessageContentDTO {
  MessageContentType messageContentType;
  std::string payload;
};

struct MessageDTO {
  std::size_t chatId;
  std::size_t messageId;
  std::string senderLogin;
  std::vector<MessageContentDTO> messageContent;
  std::int64_t timeStamp;
};

struct MessageChatDTO {
  std::size_t chatId;
  std::vector<MessageDTO> messageDTO;
};

struct ParticipantsDTO {
  std::string login;
  std::size_t lastReadMessage;
  std::vector<std::size_t> deletedMessageIds;
  bool deletedFromChat;
};
struct ChatDTO {
  std::size_t chatId;
  std::string senderLogin;
  std::vector<ParticipantsDTO> participants;
};
struct UserDTO {
  std::string login;        ///< User login.
  std::string passwordhash; ///< password hash.
  std::string userName;     ///< User's display name.
  std::string email;        ///< User's email.
  std::string phone;        ///< User's phone.
};
struct UserLoginDTO {
  std::string login; ///< User login.
};
struct UserLoginPasswordDTO {
  std::string login;        ///< User login.
  std::string passwordhash; ///< password hash.
};

struct ResponceDTO {
  bool reqResult;
  std::size_t anyNumber;
  std::string anyString;
};

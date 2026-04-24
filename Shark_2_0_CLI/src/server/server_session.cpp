#include "server_session.h"
#include "chat/chat.h"
#include "chat_system/chat_system.h"
#include "dto/dto_struct.h"
#include "exception/login_exception.h"
#include "exception/network_exception.h"
#include "message/message.h"
#include "message/message_content.h"
#include "message/message_content_struct.h"
#include "system/serialize.h"
#include "system/system_function.h"
#include "user/user.h"
#include "user/user_chat_list.h"
#include <arpa/inet.h>
#include <cstring>
#include <exception>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

ServerSession::ServerSession(ChatSystem &server) : _instance(server) {}

// getters

ServerConnectionConfig &ServerSession::getServerConnectionConfig() {
  return _serverConnectionConfig;
}

int &ServerSession::getConnection() {
  return _connection;
}
const int &ServerSession::getConnection() const {
  return _connection;
}

const std::shared_ptr<User> ServerSession::getActiveUserSrv() const {
  return _instance.getActiveUser();
}

ChatSystem &ServerSession::getInstance() {
  return _instance;
}

// setters
void ServerSession::setActiveUserSrv(const std::shared_ptr<User> &user) {
  _instance.setActiveUser(user);
}

void ServerSession::setConnection(const std::uint8_t &connection) {
  _connection = connection;
}
//
//
// transport

bool ServerSession::isConnected() const {
  return _connection > 0 && fcntl(_connection, F_GETFD) != -1;
}

void ServerSession::runServer(int socketFd) {
  try {

    // If there is already a connection, verify its validity
    if (_connection > 0) {
      // Validate the descriptor
      if (fcntl(_connection, F_GETFD) != -1) {
        // Connection is already active - nothing to do
        return;
      } else {
        // Connection exists but is invalid - close it
        close(_connection);
        _connection = -1;
      }
    }
    // Create a new connection
    struct sockaddr_in client{};
    socklen_t client_len = sizeof(client);

    _connection = accept(socketFd, (struct sockaddr *)&client, &client_len);
    if (_connection < 0) {
      throw exc::ConnectNotAcceptException();
    }

    // Saved the session number
    std::cout << "[Info] New connection established" << std::endl;

  } catch (const exc::ConnectNotAcceptException &ex) {
    std::cerr << "Server. " << ex.what() << std::endl;
  } catch (const std::exception &ex) {
    std::cerr << "Server. Unknown error. " << ex.what() << std::endl;
  }
}
//
//
//
void ServerSession::listeningClients() {

  try {
    // std::vector<std::uint8_t> buffer(MESSAGE_LENGTH);

    // Manual socket-receive diagnostics kept for transport troubleshooting.
    // // std::cerr << "[DEBUG] buffer.size() = " << buffer.size() << std::endl;

    // if (buffer.empty()) {
    //   std::cerr << "[Error] Buffer is empty (buffer.size() == 0)" << std::endl;
    //   std::exit(1);
    // }

    // if (buffer.data() == nullptr) {
    //   std::cerr << "[Error] buffer.data() == nullptr" << std::endl;
    //   std::exit(1);
    // }

    if (_connection <= 0) {
      throw exc::SocketInvalidException();
    }

    // Manual buffer-configuration diagnostics kept for transport troubleshooting.
    // if (MESSAGE_LENGTH == 0)
    //   throw exc::CreateBufferException();

    if (_connection <= 0 || fcntl(_connection, F_GETFD) == -1) {
      throw exc::SocketInvalidException();
    }
    // std::cerr << "[DEBUG] connection fd = " << connection << std::endl;

    // 1. Read 4 bytes of length
    std::uint8_t lenBuf[4];
    std::size_t total = 0;
    while (total < 4) {
      ssize_t r = recv(_connection, lenBuf + total, 4 - total, 0);
      if (r <= 0)
        throw exc::ReceiveDataException();
      total += r;
    }

    uint32_t len = 0;
    std::memcpy(&len, lenBuf, 4);
    len = ntohl(len);

    // 2. Read len bytes of data
    std::vector<std::uint8_t> buffer(len);
    std::size_t bytesReceived = 0;
    while (bytesReceived < len) {
      ssize_t r = recv(_connection, buffer.data() + bytesReceived, len - bytesReceived, 0);
      if (r <= 0)
        throw exc::ReceiveDataException();
      bytesReceived += r;
    }

    std::vector<PacketDTO> packetListReceivedVector = deSerializePacketList(buffer);

    if (packetListReceivedVector.empty())
      throw exc::EmptyPacketException();

    // Take the first packet - the header
    PacketDTO firstPacket = packetListReceivedVector.front();

    if (firstPacket.structDTOClassType != StructDTOClassType::userLoginPasswordDTO)
      throw exc::HeaderWrongTypeException();

    const auto &headerPacket = static_cast<const StructDTOClass<UserLoginPasswordDTO> &>(*firstPacket.structDTOPtr)
                                 .getStructDTOClass();

    if (headerPacket.passwordhash != "UserHeder")
      throw exc::HeaderWrongTypeException();

    if (headerPacket.login.empty())
      throw exc::HeaderWrongDataException();

    const auto &requestType = firstPacket.requestType;

    packetListReceivedVector.erase(packetListReceivedVector.begin());

    if (packetListReceivedVector.empty())
      throw exc::EmptyPacketException();

    PacketListDTO packetListReceived;
    for (const auto &packet : packetListReceivedVector)
      packetListReceived.packets.push_back(packet);

    routingRequestsFromClient(packetListReceived, requestType, _connection);
  } // try
  catch (const exc::SocketInvalidException &ex) {
    std::cerr << "Server: " << ex.what() << " = " << _connection << std::endl;
    close(_connection);
    _connection = -1;
    return;
  } catch (const exc::CreateBufferException &ex) {
    std::cerr << "Server: " << ex.what() << " = " << MESSAGE_LENGTH << std::endl;
  } catch (const exc::ReceiveDataException &ex) {
    std::cerr << "Server: " << ex.what() << std::endl;
    _connection = -1;
    return;
  } catch (const exc::HeaderWrongTypeException &ex) {
    std::cerr << "Server: " << ex.what() << std::endl;
    _connection = -1;
    return;
  } catch (const exc::HeaderWrongDataException &ex) {
    std::cerr << "Server: " << ex.what() << std::endl;
    _connection = -1;
    return;
  }
}
//
//
//
bool ServerSession::sendPacketListDTO(PacketListDTO &packetListForSend, int connection) {
  ssize_t bytesSent = 0;
  try {

    // Validate the connection
    if (connection <= 0 || fcntl(connection, F_GETFD) == -1)
      throw exc::SocketInvalidException();

    // Serialize the packets

    auto PacketSendBinary = serializePacketList(packetListForSend.packets);

    // Check: do not send an empty buffer
    if (PacketSendBinary.empty())
      throw exc::SendDataException();

    // debug check
    // std::cerr << "[DEBUG] Sending packet. Size: " << PacketSendBinary.size() << " bytes" << std::endl;
    // for (std::size_t i = 0; i < PacketSendBinary.size(); ++i) {
    //   std::cerr << static_cast<int>(PacketSendBinary[i]) << " ";
    // }
    // std::cerr << std::endl;

    // Send the data
    uint32_t len = htonl(PacketSendBinary.size());

    // std::cerr << "[SERVER DEBUG] requestType of first packet = "
    //           << static_cast<int>(packetListForSend.packets[0].requestType) << std::endl;

    send(connection, &len, 4, 0);

    // std::cerr << "[SERVER DEBUG] requestType of first packet = "
    //           << static_cast<int>(packetListForSend.packets[0].requestType) << std::endl;
    bytesSent = send(connection, PacketSendBinary.data(), PacketSendBinary.size(), 0);

    // Check the result
    if (bytesSent <= 0)
      throw exc::SendDataException();

    if (bytesSent != static_cast<ssize_t>(PacketSendBinary.size()))
      throw exc::SendDataException();
  } catch (const exc::SendDataException &ex) {
    std::cerr << "Server: " << ex.what() << std::endl;
    std::cerr << "Send failed. Bytes sent = " << bytesSent << std::endl;
    return false;
  } catch (const exc::NetworkException &ex) {
    std::cerr << "Server sendPacketListDTO: " << ex.what() << std::endl;
    return false;
  } catch (const std::exception &ex) {
    std::cerr << "Server: Unknown error. sendPacketListDTO" << ex.what() << std::endl;
    return false;
  }
  return true;
}
//
//
//
// Request processing

bool ServerSession::routingRequestsFromClient(PacketListDTO &packetListReceived,
                                              [[maybe_unused]] const RequestType &requestType,
                                              int connection) {

  const auto packetDTOrequestType = packetListReceived.packets[0].requestType;

  // check and registry User
  switch (packetDTOrequestType) {
    case RequestType::RqFrClientCheckLogin:
    case RequestType::RqFrClientCheckLogPassword:
    case RequestType::RqFrClientRegisterUser:
    case RequestType::RqFrClientFindUserByPart:
    case RequestType::RqFrClientSetLastReadMessage:
    case RequestType::RqFrClientFindUserByLogin: {

      processingCheckAndRegistryUser(packetListReceived, packetDTOrequestType, connection);
      break;
    }
    // create objects
    case RequestType::RqFrClientCreateUser:
    case RequestType::RqFrClientCreateChat:
    case RequestType::RqFrClientCreateMessage: {
      if (!processingCreateObjects(packetListReceived, packetDTOrequestType, connection))
        return false;
      break;
    }
      // get indexes and user Data
    case RequestType::RqFrClientGetUsersData: {
      processingGetIndexes(packetListReceived, packetDTOrequestType, connection);
      break;
    }
    default:
      break;
  }
  return true;
}
bool ServerSession::processingCheckAndRegistryUser(PacketListDTO &packetListReceived, const RequestType &requestType,
                                                   int connection) {

  // Built the structure of the vector of packets to send
  PacketListDTO packetDTOListForSend;
  packetDTOListForSend.packets.clear();

  switch (requestType) {
    case RequestType::RqFrClientCheckLogin: {

      // Standalone packet to send
      PacketDTO packetDTOForSend;
      packetDTOForSend.requestType = requestType;
      packetDTOForSend.structDTOClassType = StructDTOClassType::responceDTO;
      packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;

      const auto &packet = static_cast<const StructDTOClass<UserLoginDTO> &>(*packetListReceived.packets[0].structDTOPtr)
                             .getStructDTOClass();

      const auto user_ptr = _instance.findUserByLogin(packet.login);

      // Packet to send
      ResponceDTO responceDTO;
      responceDTO.anyNumber = 0;

      if (user_ptr != nullptr) {
        responceDTO.reqResult = true;
        responceDTO.anyString = packet.login;
      } else {
        responceDTO.reqResult = false;
        responceDTO.anyString = "@";
      }

      packetDTOForSend.structDTOPtr = std::make_shared<StructDTOClass<ResponceDTO>>(responceDTO);

      packetDTOListForSend.packets.push_back(packetDTOForSend);

      break;
    }
    case RequestType::RqFrClientCheckLogPassword: {
      // Standalone packet to send
      PacketDTO packetDTOForSend;
      packetDTOForSend.requestType = requestType;
      packetDTOForSend.structDTOClassType = StructDTOClassType::responceDTO;
      packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;

      const auto &packet = static_cast<const StructDTOClass<UserLoginPasswordDTO> &>(
                             *packetListReceived.packets[0].structDTOPtr)
                             .getStructDTOClass();

      const auto user_ptr = _instance.findUserByLogin(packet.login);

      // Packet to send
      ResponceDTO responceDTO;
      responceDTO.anyNumber = 0;

      if (user_ptr != nullptr) {
        responceDTO.reqResult = packet.passwordhash == user_ptr->getPasswordHash();
        responceDTO.anyString = packet.login;
      } else {
        responceDTO.reqResult = false;
        responceDTO.anyString = "@";
      }

      packetDTOForSend.structDTOPtr = std::make_shared<StructDTOClass<ResponceDTO>>(responceDTO);

      packetDTOListForSend.packets.push_back(packetDTOForSend);

      break;
    }
    case RequestType::RqFrClientRegisterUser: {

      const auto &packet = static_cast<const StructDTOClass<UserLoginDTO> &>(*packetListReceived.packets[0].structDTOPtr)
                             .getStructDTOClass();

      const auto user_ptr = _instance.findUserByLogin(packet.login);

      const auto packetListDTOVector = registerOnDeviceDataSrv(user_ptr);

      if (!packetListDTOVector.has_value())
        return false;

      packetDTOListForSend = packetListDTOVector.value();

      break;
    }
    case RequestType::RqFrClientFindUserByPart: {
      const auto &packet = static_cast<const StructDTOClass<UserLoginDTO> &>(*packetListReceived.packets[0].structDTOPtr)
                             .getStructDTOClass();

      const auto usersFound = _instance.findUserByTextPart(packet.login);

      // Packet to send

      for (const auto &user_ptr : usersFound) {
        if (user_ptr) {

          PacketDTO packetDTOForSend;
          packetDTOForSend.requestType = RequestType::RqFrClientFindUserByPart;
          packetDTOForSend.structDTOClassType = StructDTOClassType::userDTO;
          packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;

          auto userDTO = FillForSendUserDTOFromSrv(user_ptr->getLogin(), false);

          packetDTOForSend.structDTOPtr = std::make_shared<StructDTOClass<UserDTO>>(userDTO);

          packetDTOListForSend.packets.push_back(packetDTOForSend);
        }
      }

      break;
    }
    case RequestType::RqFrClientSetLastReadMessage: {
      const auto &packet = static_cast<const StructDTOClass<MessageDTO> &>(*packetListReceived.packets[0].structDTOPtr)
                             .getStructDTOClass();

      const auto &chatId = packet.chatId;
      const auto &messageId = packet.messageId;
      const auto &userLogin = packet.senderLogin;

      const auto user_ptr = _instance.findUserByLogin(userLogin);
      const auto chat_ptr = _instance.getChatById(chatId);

      ResponceDTO responceDTO;
      responceDTO.anyNumber = 0;
      responceDTO.anyString = "";

      // Packet to send
      PacketDTO packetDTOForSend;
      packetDTOForSend.requestType = RequestType::RqFrClientSetLastReadMessage;
      packetDTOForSend.structDTOClassType = StructDTOClassType::responceDTO;
      packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;

      if (user_ptr == nullptr || chat_ptr == nullptr) {
        std::cerr << "Server: RqFrClientSetLastReadMessage. No user or chat" << std::endl;
        responceDTO.reqResult = false;
      } else {
        chat_ptr->setLastReadMessageId(user_ptr, messageId);
        responceDTO.reqResult = true;
      }

      packetDTOForSend.structDTOPtr = std::make_shared<StructDTOClass<ResponceDTO>>(responceDTO);
      packetDTOListForSend.packets.push_back(packetDTOForSend);
      break;
    }

    default:
      throw exc::HeaderWrongTypeException();
      break;
  }

  sendPacketListDTO(packetDTOListForSend, connection);
  //   } // try
  //   catch (const exc::UserNotFoundException &ex) {
  //     return false;
  //   } catch (const exc::ChatNotFoundException &ex) {
  //     std::cerr << "Server: RqFrClientSetLastReadMessage. " << ex.what() << std::endl;
  //     return false;
  //   }
  return true;
}
//
//
//
bool ServerSession::processingCreateObjects(PacketListDTO &packetListReceived, const RequestType &requestType,
                                            int connection) { // Built the structure of the vector of packets to send

  PacketListDTO packetDTOListForSend;
  packetDTOListForSend.packets.clear();

  try {
    switch (requestType) {
      case RequestType::RqFrClientCreateUser: {

        if (packetListReceived.packets.empty() ||
            packetListReceived.packets[0].structDTOClassType != StructDTOClassType::userDTO) {
          throw exc::EmptyPacketException();
        }

        // Standalone packet to send
        PacketDTO packetDTOForSend;
        packetDTOForSend.requestType = requestType;
        packetDTOForSend.structDTOClassType = StructDTOClassType::responceDTO;
        packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;

        const auto &packet = static_cast<const StructDTOClass<UserDTO> &>(*packetListReceived.packets[0].structDTOPtr)
                               .getStructDTOClass();

        // Packet to send
        ResponceDTO responceDTO;
        responceDTO.anyNumber = 0;
        responceDTO.anyString = "";

        if (createUserSrv(packet)) {
          responceDTO.reqResult = true;
        } else
          responceDTO.reqResult = false;

        packetDTOForSend.structDTOPtr = std::make_shared<StructDTOClass<ResponceDTO>>(responceDTO);

        packetDTOListForSend.packets.push_back(packetDTOForSend);

        break;
      }
      case RequestType::RqFrClientCreateChat: {
        // Standalone packet to send
        PacketDTO packetDTOForSend;
        packetDTOForSend.requestType = requestType;
        packetDTOForSend.structDTOClassType = StructDTOClassType::responceDTO;
        packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;

        // Add the chat
        const auto &packet = static_cast<const StructDTOClass<ChatDTO> &>(*packetListReceived.packets[0].structDTOPtr)
                               .getStructDTOClass();

        // Packet to send
        ResponceDTO responceDTO;

        auto chat_ptr = std::make_shared<Chat>();

        if (createNewChatSrv(chat_ptr, packet)) {

          // Add the message
          auto &packet = static_cast<StructDTOClass<MessageChatDTO> &>(*packetListReceived.packets[1].structDTOPtr)
                           .getStructDTOClass();

          packet.chatId = chat_ptr->getChatId();

          if (createNewMessageChatSrv(chat_ptr, packet)) {
            responceDTO.reqResult = true;
            responceDTO.anyNumber = chat_ptr->getChatId();
            responceDTO.anyString = std::to_string(chat_ptr->getMessages().begin()->second->getMessageId());

            packetDTOForSend.structDTOPtr = std::make_shared<StructDTOClass<ResponceDTO>>(responceDTO);

            packetDTOListForSend.packets.push_back(packetDTOForSend);
          } // if message
          else {
            responceDTO.reqResult = false;
            responceDTO.anyString = "Message false";
          }
        } // if chat
        else {
          responceDTO.reqResult = false;
          responceDTO.anyNumber = -1;
        }
        break;
      }
      case RequestType::RqFrClientCreateMessage: {

        // Standalone packet to send
        PacketDTO packetDTOForSend;
        packetDTOForSend.requestType = requestType;
        packetDTOForSend.structDTOClassType = StructDTOClassType::responceDTO;
        packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;

        // Packet to send
        ResponceDTO responceDTO;
        responceDTO.anyNumber = 0;

        // Extract the packet
        const auto &messageDTO = static_cast<const StructDTOClass<MessageDTO> &>(
                                   *packetListReceived.packets[0].structDTOPtr)
                                   .getStructDTOClass();

        const auto &result = createNewMessageSrv(messageDTO);

        if (result) {
          responceDTO.reqResult = true;
          responceDTO.anyString = std::to_string(result);

          packetDTOForSend.structDTOPtr = std::make_shared<StructDTOClass<ResponceDTO>>(responceDTO);

          packetDTOListForSend.packets.push_back(packetDTOForSend);
        } // if result
        else {
          responceDTO.reqResult = false;
          responceDTO.anyString = "0";
        }

        break;
      }
      default:
        throw exc::HeaderWrongTypeException();
        break;
    }
    // }// for
    sendPacketListDTO(packetDTOListForSend, connection);
  } // try
  catch (const exc::SendDataException &ex) {
    std::cerr << "Server: " << ex.what() << std::endl;
    return false;
  } catch (const exc::NetworkException &ex) {
    std::cerr << "Server: " << ex.what() << std::endl;
    return false;

  } catch (const std::bad_cast &ex) {
    std::cerr << "Server: Wrong packet type. " << ex.what() << std::endl;
    return false;
  }
  return true;
}
//
//
//
bool ServerSession::processingGetIndexes(PacketListDTO &packetListReceived,
                                         [[maybe_unused]] const RequestType &requestType,
                                         int connection) {
  // Built the structure of the vector of packets to send
  PacketListDTO packetDTOListForSend;

  try {
    for (const auto &packetDTOReceived : packetListReceived.packets) {

      if (packetDTOReceived.requestType != RequestType::RqFrClientGetUsersData)
        throw exc::HeaderWrongTypeException();

      const auto &packet = static_cast<const StructDTOClass<UserLoginDTO> &>(*packetDTOReceived.structDTOPtr)
                             .getStructDTOClass();

      const auto user_ptr = _instance.findUserByLogin(packet.login);

      if (!user_ptr)
        throw exc::UserNotFoundException();

      UserDTO userDTO = FillForSendUserDTOFromSrv(user_ptr->getLogin(), false);

      // Build the user
      PacketDTO packetDTO;
      packetDTO.requestType = RequestType::RqFrClientGetUsersData;
      packetDTO.structDTOClassType = StructDTOClassType::userDTO;
      packetDTO.reqDirection = RequestDirection::ClientToSrv;
      packetDTO.structDTOPtr = std::make_shared<StructDTOClass<UserDTO>>(userDTO);

      packetDTOListForSend.packets.push_back(packetDTO);
    }

    sendPacketListDTO(packetDTOListForSend, connection);
  } // try
  catch (const exc::SendDataException &ex) {
    std::cerr << "Server: " << ex.what() << std::endl;
    return false;
  } catch (const exc::HeaderWrongTypeException &ex) {
    std::cerr << "Server: " << ex.what() << std::endl;
    return false;
  } catch (const exc::UserNotFoundException &ex) {
    std::cerr << "Server: " << ex.what() << std::endl;
    return false;
  }
  return true;
}
//
//
//
bool ServerSession::processingReceivedQueue([[maybe_unused]] const std::string &userLogin) {

  // auto &dequeReceived = _instance.getPacketReceivedDeque();
  // auto &dequeForSend = _instance.getPacketForSendDeque();

  // if (dequeReceived.empty())
  //   return false;

  // while (!dequeReceived.empty()) {
  //   const auto &[packetDTO, userLogin] = dequeReceived.front();

  //   PacketListDTO packetListForSend;

  //   if (processingRequestToToSendToServer(packetDTO, packetListForSend))
  //     dequeForSend.push_back({packetListForSend, userLogin});
  //   dequeReceived.pop_front();
  //   ;
  // }
  return true;
}
//
//
//
bool ServerSession::processingSendQueue() {

  // auto &dequeForSend = _instance.getPacketForSendDeque();
  // try {
  //   while (!dequeForSend.empty()) {
  //     auto &[packetListDTO, userLogin] = dequeForSend.front();

  //     auto PacketSendBinary = serializePacketList(packetListDTO.packets);
  //     auto it = _loginToConnectionMap.find(userLogin);

  //     if (it == _loginToConnectionMap.end()) {
  //       std::cerr << "Server: login " << userLogin
  //                 << " not found in loginToConnectionMap. Skipping
  //                 the send."
  //                 << std::endl;
  //       dequeForSend.pop_front();
  //       continue;
  //     }

  //     int connection = it->second;

  //     ssize_t bytesSent =
  //         send(connection, PacketSendBinary.data(),
  //         PacketSendBinary.size(), 0);
  //     if (bytesSent <= 0) {
  //       throw exc::SendDataException();
  //     } else {
  //       dequeForSend.pop_front();
  //     }
  //   }
  // } // try
  // catch (const exc::SendDataException &ex) {
  //   std::cerr << "Server: " << ex.what() << std::endl;
  //   return false;
  // }

  return true;
}
//
//
//

// User search
const std::vector<UserDTO> ServerSession::findUserByTextPartFromSrv(const std::string &textToFind) const {

  const auto &users = this->_instance.findUserByTextPart(textToFind);
  std::vector<UserDTO> userDTO;

  if (!users.empty()) {
    for (const auto &user : users) {
      userDTO.push_back(FillForSendUserDTOFromSrv(user->getLogin(), true));
    }
  } else
    userDTO.clear();
  return userDTO;
}

bool ServerSession::checkUserLoginSrv(const UserLoginDTO &userLoginDTO) const {

  if (this->_instance.findUserByLogin(userLoginDTO.login) != nullptr)
    return true;
  else
    return false;
}
//
//
//
bool ServerSession::checkUserPasswordSrv(const UserLoginPasswordDTO &userLoginPasswordDTO) const {

  return this->_instance.checkPasswordValidForUser(userLoginPasswordDTO.passwordhash, userLoginPasswordDTO.login);
}
//
//
//
UserDTO ServerSession::FillForSendUserDTOFromSrv(const std::string &userLogin, bool loginUser) const {

  UserDTO userDTO;
  auto user = this->_instance.findUserByLogin(userLogin);
  if (!user)
    throw exc::UserNotFoundException();

  userDTO.login = user->getLogin();
  userDTO.userName = user->getUserName();
  userDTO.email = user->getEmail();
  userDTO.phone = user->getPhone();
  userDTO.passwordhash = loginUser ? user->getPasswordHash() : "-1";

  return userDTO;
}
//
//
//
// Get a single user chat
std::optional<ChatDTO> ServerSession::FillForSendOneChatDTOFromSrv(const std::shared_ptr<Chat> &chat_ptr,
                                                                   const std::shared_ptr<User> &user) {
  ChatDTO chatDTO;

  // Took the chatId
  chatDTO.chatId = chat_ptr->getChatId();
  chatDTO.senderLogin = user->getLogin();

  try {
    // Get the list of participants
    auto participants = chat_ptr->getParticipants();

    // Iterate over participants
    for (const auto &participant : participants) {

      // Got a pointer to the user
      const auto user_ptr = participant._user.lock();

      // Temporary struct to fill in
      ParticipantsDTO participantsDTO;

      if (user_ptr) {

        // Fill in the user data for registration in the system

        participantsDTO.login = user_ptr->getLogin();

        participantsDTO.lastReadMessage = chat_ptr->getLastReadMessageId(user_ptr);

        // Fill deletedMessageIds
        const auto &delMsgMap = chat_ptr->getDeletedMessagesMap();
        const auto it = delMsgMap.find(participantsDTO.login);

        if (it != delMsgMap.end()) {

          for (const auto &delMsgId : it->second)
            participantsDTO.deletedMessageIds.push_back(delMsgId);
        }

        participantsDTO.deletedFromChat = participant._deletedFromChat;

        chatDTO.participants.push_back(participantsDTO);

      } // if user_ptr
      else
        throw exc::UserNotFoundException();
    } // for participants
  }
  // try
  catch (const exc::UserNotFoundException &ex) {
    std::cerr << "Server: FillForSendOneChatDTOFromSrv. " << ex.what() << std::endl;
    return std::nullopt;
  }

  return chatDTO;
}
//
//
// Get all chats for a user
std::optional<std::vector<ChatDTO>> ServerSession::FillForSendAllChatDTOFromSrv(const std::shared_ptr<User> &user) {
  // Took the chat list
  auto chatList = user->getUserChatList();
  std::vector<ChatDTO> chatDTOVector;
  // Iterate over chats in the chat list
  for (const auto &chat : chatList->getChatFromList()) {

    // Got a pointer to the chat
    auto chat_ptr = chat.lock();

    if (!chat_ptr) {
      std::cerr << "Server: FillForSendAllChatDTOFromSrv. Chat is not available" << std::endl;
      continue;
    } else {
      auto tempDTO = FillForSendOneChatDTOFromSrv(chat_ptr, user);

      if (tempDTO.has_value())
        chatDTOVector.push_back(tempDTO.value());
      else {
        std::cerr << "Server: FillForSendAllChatDTOFromSrv. Chat is not filled in" << std::endl;
        continue;
      }
    } // first if chat_ptr
  } // first for

  return chatDTOVector;
}
//
//
// Get a specific user message
std::optional<MessageDTO> ServerSession::FillForSendOneMessageDTOFromSrv(const std::shared_ptr<Message> &message,
                                                                         const std::size_t &chatId) {

  MessageDTO messageDTO;
  auto user_ptr = message->getSender().lock();

  messageDTO.senderLogin = user_ptr ? user_ptr->getLogin() : "";

  messageDTO.chatId = chatId;
  messageDTO.messageId = message->getMessageId();
  messageDTO.timeStamp = message->getTimeStamp();

  // Get the content
  MessageContentDTO temContent;
  temContent.messageContentType = MessageContentType::Text;
  auto contentElement = message->getContent().front();

  auto contentTextPtr = std::dynamic_pointer_cast<MessageContent<TextContent>>(contentElement);

  if (contentTextPtr) {
    auto contentText = contentTextPtr->getMessageContent();
    temContent.payload = contentText._text;
  }

  messageDTO.messageContent.push_back(temContent);

  return messageDTO;
}
//
//
// Get messages of a specific user chat
std::optional<MessageChatDTO> ServerSession::fillForSendChatMessageDTOFromSrv(const std::shared_ptr<Chat> &chat) {

  MessageChatDTO messageChatDTO;

  // Took the chatId
  messageChatDTO.chatId = chat->getChatId();
  try {
    for (const auto &message : chat->getMessages()) {

      const auto &messageDTO = FillForSendOneMessageDTOFromSrv(message.second, messageChatDTO.chatId);

      if (!messageDTO.has_value())
        throw exc::MessagesNotFoundException();

      messageChatDTO.messageDTO.push_back(messageDTO.value());

    } // for message
  } // try
  catch (const exc::MessagesNotFoundException &ex) {
    std::cerr << "Server: fillForSendChatMessageDTOFromSrv. " << ex.what() << std::endl;
    return std::nullopt;
  }
  return messageChatDTO;
}
//
//
// Get all messages for a user
std::optional<std::vector<MessageChatDTO>> ServerSession::fillForSendAllMessageDTOFromSrv(
  const std::shared_ptr<User> &user) {

  std::vector<MessageChatDTO> messageChatDTOVector;
  try {
    // Took the chat list
    auto chatList = user->getUserChatList();

    // Iterate over chats in the chat list
    for (const auto &chat : chatList->getChatFromList()) {

      // Got a pointer to the chat
      auto chat_ptr = chat.lock();

      if (chat_ptr) {

        // Invoke fetching of chat messages

        const auto &messageChatDTO = fillForSendChatMessageDTOFromSrv(chat_ptr);

        if (!messageChatDTO.has_value())
          throw exc::CreateMessageException();

        messageChatDTOVector.push_back(messageChatDTO.value());

      } // if chat_ptr
      else
        throw exc::ChatNotFoundException();
    } // first if chat_ptr
  } // try
  catch (const exc::ChatNotFoundException &ex) {
    std::cerr << "Server: fillForSendAllMessageDTOFromSrv. " << ex.what() << std::endl;
    return std::nullopt;
  } catch (const exc::CreateMessageException &ex) {
    std::cerr << "Server: fillForSendAllMessageDTOFromSrv. " << ex.what() << std::endl;
    return std::nullopt;
  }
  return messageChatDTOVector;
}
//
//
//

// utilities

void ServerSession::runUDPServerDiscovery(std::uint16_t listenPort) {
  int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

  std::cout << "[DEBUG] runUDPServerDiscovery started" << std::endl;

  try {
    if (udpSocket < 0)
      throw exc::CreateSocketTypeException();

    // REQUIRED: enable reuse + broadcast
    int enable = 1;
    setsockopt(udpSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(listenPort);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udpSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
      std::cerr << "UDP: Failed to bind\n";
      close(udpSocket);
      return;
    }

    std::cout << "[UDP] Server is listening for UDP discovery on port " << listenPort << std::endl;

    while (true) {
      char buffer[128] = {0};
      sockaddr_in clientAddr{};
      socklen_t addrLen = sizeof(clientAddr);

      ssize_t bytesReceived = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0,
                                       (sockaddr *)&clientAddr, &addrLen);

      if (bytesReceived > 0 && std::string(buffer) == "ping?") {
        const char *reply = "pong";
        sendto(udpSocket, reply, std::strlen(reply), 0, (sockaddr *)&clientAddr, addrLen);
      }
    }

    close(udpSocket);
  } catch (const exc::CreateSocketTypeException &ex) {
    std::cerr << "UDP: " << ex.what() << std::endl;
    close(udpSocket);
  }
}

bool ServerSession::createUserSrv(const UserDTO &userDTO) const {

  auto user = std::make_shared<User>(
    UserData(userDTO.login, userDTO.userName, userDTO.passwordhash, userDTO.email, userDTO.phone));

  user->createChatList(std::make_shared<UserChatList>(user));
  this->_instance.addUserToSystem(user);

  return true;
}
//
//
//
std::size_t ServerSession::createNewChatSrv(std::shared_ptr<Chat> &chat_ptr, ChatDTO chatDTO) const {

  auto newChatId = _instance.createNewChatId(chat_ptr);
  try {
    // Initialize the chat and find the user
    //   auto user_ptr = this->_instance.findUserByLogin(chatDTO.login);

    // Fill in the fields
    // Create a global chat number
    chat_ptr->setChatId(newChatId);

    // Add participants
    // Add the chat to each participant's chat list

    for (const auto &participant : chatDTO.participants) {
      auto participant_ptr = this->_instance.findUserByLogin(participant.login);

      if (participant_ptr == nullptr) {
        throw exc::UserNotFoundException();
      } else {
        chat_ptr->addParticipant(participant_ptr, participant.lastReadMessage, participant.deletedFromChat);
      }
    }

    // Add the chat to the system
    this->_instance.addChatToInstance(chat_ptr);

    // ⬇⬇⬇ Check
    // std::cout << "\n[DEBUG] Added chat on the server:\n";

    // const auto &chats = this->_instance.getChats();
    // std::shared_ptr<Chat> chat_ptr_test;

    // for (const auto &chat : chats)
    //   if (chat->getChatId() == newChatId) {
    //     chat_ptr_test = chat;
    //   }

    // if (!chat_ptr_test)
    //   throw exc::InternalDataErrorException();

    // std::cout << "participants:" << std::endl;
    // for (const auto &participant : chat_ptr->getParticipants()) {
    //   auto user = participant._user.lock();
    //   if (user) {
    //     std::cout << "- " << user->getLogin() << " | deleted: " << participant._deletedFromChat << std::endl;
    //   } else {
    //     throw exc::UserNotFoundException();
    //   }
    // }
  } // try
  catch (const exc::UserNotFoundException &ex) {
    std::cerr << "Server: createNewChatSrv The user has already been removed. " << ex.what() << std::endl;
    _instance.moveToFreeChatIdSrv(newChatId);
    return 0;
  }
  return newChatId;
}
//
//
//
std::size_t ServerSession::createNewMessageSrv(const MessageDTO &messageDTO) const {
  try {
    const auto &chat_ptr = this->_instance.getChatById(messageDTO.chatId);
    if (!chat_ptr)
      throw exc::ChatNotFoundException();

    auto sender_ptr = this->_instance.findUserByLogin(messageDTO.senderLogin);
    if (!sender_ptr)
      throw exc::UserNotFoundException();

    if (messageDTO.messageContent.empty())
      throw exc::CreateMessageException();

    auto message_ptr = std::make_shared<Message>(
      createOneMessage(messageDTO.messageContent[0].payload, sender_ptr, messageDTO.timeStamp, 0));

    chat_ptr->addMessageToChat(message_ptr, sender_ptr, true);

    const auto &t = message_ptr->getMessageId();

    return t;

  } catch (const exc::UserNotFoundException &ex) {
    std::cerr << "Server: createNewMessageSrv: user not found: " << ex.what() << std::endl;
    return 0;
  } catch (const exc::ChatNotFoundException &ex) {
    std::cerr << "Server: createNewMessageSrv: chat not found: " << ex.what() << std::endl;
    return 0;
  } catch (const exc::CreateMessageException &ex) {
    std::cerr << "Server: createNewMessageSrv: message is empty: " << ex.what() << std::endl;
    return 0;
  }
}

//
//
//
bool ServerSession::createNewMessageChatSrv([[maybe_unused]] std::shared_ptr<Chat> &chat,
                                            MessageChatDTO &messageChatDTO) {
  // Add messages from the chat
  for (auto &messageDTO : messageChatDTO.messageDTO) {
    messageDTO.chatId = messageChatDTO.chatId;
    createNewMessageSrv(messageDTO);
  }
  return true;
}
//
//
// Build and return user data, chats and messages in response to the request
std::optional<PacketListDTO> ServerSession::registerOnDeviceDataSrv(const std::shared_ptr<User> &user) {

  PacketListDTO packetListDTO;

  const auto login = user->getLogin();

  UserDTO userDTO;
  userDTO = FillForSendUserDTOFromSrv(login, true);

  // Build the user
  PacketDTO packetDTO;
  packetDTO.requestType = RequestType::RqFrClientRegisterUser;
  packetDTO.structDTOClassType = StructDTOClassType::userDTO;
  packetDTO.reqDirection = RequestDirection::ClientToSrv;
  packetDTO.structDTOPtr = std::make_shared<StructDTOClass<UserDTO>>(userDTO);

  packetListDTO.packets.push_back(packetDTO);

  // Build the chats
  //   std::vector<ChatDTO> chatDTO;
  //   std::unordered_set<std::shared_ptr<User>> participantLogins;

  auto chatDTOVector = FillForSendAllChatDTOFromSrv(user);

  if (!chatDTOVector.has_value())
    return std::nullopt;

  for (const auto &pct : chatDTOVector.value()) {
    PacketDTO packetDTO;
    packetDTO.requestType = RequestType::RqFrClientRegisterUser;
    packetDTO.structDTOClassType = StructDTOClassType::chatDTO;
    packetDTO.reqDirection = RequestDirection::ClientToSrv;
    packetDTO.structDTOPtr = std::make_shared<StructDTOClass<ChatDTO>>(pct);

    packetListDTO.packets.push_back(packetDTO);
  }

  // Build the messages

  const auto &messageChatDTO = fillForSendAllMessageDTOFromSrv(user);

  if (!messageChatDTO.has_value())
    return std::nullopt;

  for (const auto &pct : messageChatDTO.value()) {
    PacketDTO packetDTO;
    packetDTO.requestType = RequestType::RqFrClientRegisterUser;
    packetDTO.structDTOClassType = StructDTOClassType::messageChatDTO;
    packetDTO.reqDirection = RequestDirection::ClientToSrv;
    packetDTO.structDTOPtr = std::make_shared<StructDTOClass<MessageChatDTO>>(pct);

    packetListDTO.packets.push_back(packetDTO);
  }

  for (std::size_t i = 0; i < packetListDTO.packets.size(); ++i) {
    std::cerr << "[PACKET " << i << "] type = " << static_cast<int>(packetListDTO.packets[i].structDTOClassType)
              << std::endl;
  }
  return packetListDTO;
}

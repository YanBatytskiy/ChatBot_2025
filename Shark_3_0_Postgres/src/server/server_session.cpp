#include "server_session.h"
#include "dto/dto_struct.h"
#include "core/exception/login_exception.h"
#include "core/exception/network_exception.h"
#include "core/exception/sql_exception.h"
#include "core/exception/validation_exception.h"
#include "core/message/message.h"
#include "sql_server.h"
#include "core/system/serialize.h"
#include "core/system/system_function.h"
#include <arpa/inet.h>
#include <cstddef>
#include <cstring>
#include <exception>
#include <fcntl.h>
#include <iostream>
#include <iterator>
#include <libpq-fe.h>
#include <memory>
#include <optional>
#include <unistd.h>
#include <vector>

// ServerSession::ServerSession(ChatSystem &server) : _instance(server) {}

// getters
PGconn *ServerSession::getPGConnection() {
  return _pqConnection;
}
const PGconn *ServerSession::getPGConnection() const {
  return _pqConnection;
}

ServerConnectionConfig &ServerSession::getServerConnectionConfig() {
  return _serverConnectionConfig;
}

int &ServerSession::getConnection() {
  return _connection;
}
const int &ServerSession::getConnection() const {
  return _connection;
}

// setters
void ServerSession::setPgConnection(PGconn *connection) {
  _pqConnection = connection;
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

    // If a connection already exists, verify its validity
    if (_connection > 0) {
      // Check the descriptor's validity
      if (fcntl(_connection, F_GETFD) != -1) {
        // Connection is already active; nothing to do
        return;
      } else {
        // Connection exists but is invalid; close it
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

    // // Diagnostic block kept for buffer-shape troubleshooting.
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

    // Diagnostic block kept for receive-loop guard checks.
    // if (MESSAGE_LENGTH == 0)
    //   throw exc::CreateBufferException();

    if (_connection <= 0 || fcntl(_connection, F_GETFD) == -1) {
      throw exc::SocketInvalidException();
    }
    // Diagnostic block kept for connection-state inspection.

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

    // Diagnostic block kept for outgoing packet inspection.
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
      processingGetUserData(packetListReceived, packetDTOrequestType, connection);
      break;
    }
    default:
      break;
  }
  return true;
}
bool ServerSession::processingCheckAndRegistryUser(PacketListDTO &packetListReceived, const RequestType &requestType,
                                                   int connection) {

  // Created the outgoing packet vector structure
  PacketListDTO packetDTOListForSend;
  packetDTOListForSend.packets.clear();

  switch (requestType) {
    case RequestType::RqFrClientCheckLogin: {

      // Individual outgoing packet
      PacketDTO packetDTOForSend;
      packetDTOForSend.requestType = requestType;
      packetDTOForSend.structDTOClassType = StructDTOClassType::responceDTO;
      packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;

      const auto &packet = static_cast<const StructDTOClass<UserLoginDTO> &>(*packetListReceived.packets[0].structDTOPtr)
                             .getStructDTOClass();

      // Outgoing packet
      ResponceDTO responceDTO;
      responceDTO.anyNumber = 0;

      if (checkUserLoginSrvSQL(packet.login)) {
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
      // Individual outgoing packet
      PacketDTO packetDTOForSend;
      packetDTOForSend.requestType = requestType;
      packetDTOForSend.structDTOClassType = StructDTOClassType::responceDTO;
      packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;

      const auto &packet = static_cast<const StructDTOClass<UserLoginPasswordDTO> &>(
                             *packetListReceived.packets[0].structDTOPtr)
                             .getStructDTOClass();

      // Outgoing packet
      ResponceDTO responceDTO;
      responceDTO.anyNumber = 0;

      if (checkUserPasswordSrvSql(packet)) {
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
    case RequestType::RqFrClientRegisterUser: {

      const auto &packet = static_cast<const StructDTOClass<UserLoginDTO> &>(*packetListReceived.packets[0].structDTOPtr)
                             .getStructDTOClass();

      const auto packetListDTOVector = registerOnDeviceDataSrvSQL(packet.login);

      if (!packetListDTOVector.has_value())
        return false;

      packetDTOListForSend = packetListDTOVector.value();

      break;
    }
    case RequestType::RqFrClientFindUserByPart: {
      const auto &packet = static_cast<const StructDTOClass<UserLoginPasswordDTO> &>(
                             *packetListReceived.packets[0].structDTOPtr)
                             .getStructDTOClass();

      auto userDTOVector = getUsersByTextPartSQL(this->getPGConnection(), packet);

      if (!userDTOVector.has_value())
        return false;

      // Outgoing packet
      PacketDTO packetDTOForSend;
      packetDTOForSend.requestType = RequestType::RqFrClientFindUserByPart;
      packetDTOForSend.structDTOClassType = StructDTOClassType::userDTO;
      packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;

      for (const auto &userDTO : userDTOVector.value()) {

        packetDTOForSend.structDTOPtr = std::make_shared<StructDTOClass<UserDTO>>(userDTO);

        packetDTOListForSend.packets.push_back(packetDTOForSend);
      }

      break;
    }
    case RequestType::RqFrClientSetLastReadMessage: {
      const auto &packet = static_cast<const StructDTOClass<MessageDTO> &>(*packetListReceived.packets[0].structDTOPtr)
                             .getStructDTOClass();

      auto value = setLastReadMessageSQL(this->getPGConnection(), packet);

      ResponceDTO responceDTO;
      responceDTO.anyNumber = 0;
      responceDTO.anyString = "";

      // Outgoing packet
      PacketDTO packetDTOForSend;
      packetDTOForSend.requestType = RequestType::RqFrClientSetLastReadMessage;
      packetDTOForSend.structDTOClassType = StructDTOClassType::responceDTO;
      packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;

      if (!value) {
        std::cerr << "Server: RqFrClientSetLastReadMessage. Database error. Value was not set." << std::endl;
        responceDTO.reqResult = false;
      } else {
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
  return true;
}
//
//
//
bool ServerSession::processingCreateObjects(PacketListDTO &packetListReceived, const RequestType &requestType,
                                            int connection) { // Created the outgoing packet vector structure

  PacketListDTO packetDTOListForSend;
  packetDTOListForSend.packets.clear();

  try {
    switch (requestType) {
      case RequestType::RqFrClientCreateUser: {

        if (packetListReceived.packets.empty() ||
            packetListReceived.packets[0].structDTOClassType != StructDTOClassType::userDTO) {
          throw exc::EmptyPacketException();
        }

        // Individual outgoing packet
        PacketDTO packetDTOForSend;
        packetDTOForSend.requestType = requestType;
        packetDTOForSend.structDTOClassType = StructDTOClassType::responceDTO;
        packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;

        const auto &packet = static_cast<const StructDTOClass<UserDTO> &>(*packetListReceived.packets[0].structDTOPtr)
                               .getStructDTOClass();

        // Outgoing packet
        ResponceDTO responceDTO;
        responceDTO.anyNumber = 0;
        responceDTO.anyString = "";

        if (createUserSQL(this->getPGConnection(), packet)) {
          responceDTO.reqResult = true;
        } else
          responceDTO.reqResult = false;

        packetDTOForSend.structDTOPtr = std::make_shared<StructDTOClass<ResponceDTO>>(responceDTO);

        packetDTOListForSend.packets.push_back(packetDTOForSend);

        break;
      }
      case RequestType::RqFrClientCreateChat: {
        // Individual outgoing packet
        PacketDTO packetDTOForSend;
        packetDTOForSend.requestType = requestType;
        packetDTOForSend.structDTOClassType = StructDTOClassType::responceDTO;
        packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;

        // Take the chat
        const auto &packetChat = static_cast<const StructDTOClass<ChatDTO> &>(*packetListReceived.packets[0].structDTOPtr)
                                   .getStructDTOClass();

        // Take the message
        auto &packetMessage = static_cast<StructDTOClass<MessageChatDTO> &>(*packetListReceived.packets[1].structDTOPtr)
                                .getStructDTOClass();

        // Outgoing packet
        ResponceDTO responceDTO;

        const auto &result = (createChatAndMessageSQL(this->getPGConnection(), packetChat, packetMessage));

        if (result.size() > 0) {

          responceDTO.reqResult = true;
          // chat_id
          responceDTO.anyNumber = static_cast<size_t>(std::stoull(result[1]));
          // message_id
          responceDTO.anyString = result[0];

          packetDTOForSend.structDTOPtr = std::make_shared<StructDTOClass<ResponceDTO>>(responceDTO);

          packetDTOListForSend.packets.push_back(packetDTOForSend);
        } else {
          responceDTO.reqResult = false;
          responceDTO.anyString = "";
          responceDTO.anyNumber = 0;
        }
        break;
      }
      case RequestType::RqFrClientCreateMessage: {

        // Individual outgoing packet
        PacketDTO packetDTOForSend;
        packetDTOForSend.requestType = requestType;
        packetDTOForSend.structDTOClassType = StructDTOClassType::responceDTO;
        packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;

        // Outgoing packet
        ResponceDTO responceDTO;
        responceDTO.anyNumber = 0;

        // Extract the packet
        const auto &messageDTO = static_cast<const StructDTOClass<MessageDTO> &>(
                                   *packetListReceived.packets[0].structDTOPtr)
                                   .getStructDTOClass();

        const auto &result = createMessageSQL(this->getPGConnection(), messageDTO);

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
bool ServerSession::processingGetUserData(PacketListDTO &packetListReceived,
                                          [[maybe_unused]] const RequestType &requestType,
                                          int connection) {

  // Vector of logins for the database query
  std::vector<std::string> logins;
  logins.clear();

  // Created the outgoing packet vector structure
  PacketListDTO packetDTOListForSend;

  try {
    // Fill the login vector for the database query
    for (const auto &packetDTOReceived : packetListReceived.packets) {

      if (packetDTOReceived.requestType != RequestType::RqFrClientGetUsersData)
        throw exc::HeaderWrongTypeException();

      const auto &packet = static_cast<const StructDTOClass<UserLoginDTO> &>(*packetDTOReceived.structDTOPtr)
                             .getStructDTOClass();

      auto result = checkUserLoginSrvSQL(packet.login);

      if (!result)
        throw exc::UserNotFoundException();

      logins.push_back(packet.login);
    } // for

    // Get the array of users
    if (logins.size() == 0)
      throw exc::UserNotFoundException();

    auto userDTOVector = FillForSendSeveralUsersDTOFromSrvSQL(logins);

    if (!userDTOVector.has_value())
      throw exc::UserNotFoundException();

    for (const auto &userDTO : userDTOVector.value()) {

      // Build the user
      PacketDTO packetDTO;
      packetDTO.requestType = RequestType::RqFrClientGetUsersData;
      packetDTO.structDTOClassType = StructDTOClassType::userDTO;
      packetDTO.reqDirection = RequestDirection::ClientToSrv;
      packetDTO.structDTOPtr = std::make_shared<StructDTOClass<UserDTO>>(userDTO);

      packetDTOListForSend.packets.push_back(packetDTO);
    } // for user

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
bool ServerSession::checkUserLoginSrvSQL(const std::string &login) {

  PGresult *result;

  std::string sql = "";

  try {

    std::string loginEsc = login;
    for (std::size_t pos = 0; (pos = loginEsc.find('\'', pos)) != std::string::npos; pos += 2) {
      loginEsc.replace(pos, 1, "''");
    }

    sql = R"(select id from public.users as u where u.login = ')";
    sql += loginEsc + "';";

    result = execSQL(this->getPGConnection(), sql);

    if (result == nullptr)
      throw exc::SQLSelectException(", FindUserByLoginSrv");

    if (PQresultStatus(result) == PGRES_TUPLES_OK && PQntuples(result) > 0) {
      PQclear(result);
      return true;
    } else {
      return false;
    }
  } // try
  catch (const exc::SQLSelectException &ex) {
    std::cerr << "Server: " << ex.what() << std::endl;
    return false;
  }
}
//
//
//
bool ServerSession::checkUserPasswordSrvSql(const UserLoginPasswordDTO &userLoginPasswordDTO) {
  PGresult *result;

  std::string sql = "";

  try {

    std::string loginEsc = userLoginPasswordDTO.login;
    for (std::size_t pos = 0; (pos = loginEsc.find('\'', pos)) != std::string::npos; pos += 2) {
      loginEsc.replace(pos, 1, "''");
    }

    std::string passwordHashEsc = userLoginPasswordDTO.passwordhash;
    for (std::size_t pos = 0; (pos = passwordHashEsc.find('\'', pos)) != std::string::npos; pos += 2) {
      passwordHashEsc.replace(pos, 1, "''");
    }

    sql = R"(with user_record as (
		select id as user_id 
		from public.users 
		where login = ')";
    sql += loginEsc + "')";

    sql += R"(select password_hash from public.users_passhash as ph join user_record ur on ph
	.user_id = ur.user_id where password_hash = ')";
    sql += passwordHashEsc + "';";

    result = execSQL(this->getPGConnection(), sql);

    if (result == nullptr)
      throw exc::SQLSelectException(", checkUserPasswordSrvSql");

    if (PQresultStatus(result) == PGRES_TUPLES_OK && PQntuples(result) > 0) {
      PQclear(result);
      return true;
    } else {
      return false;
    }
  } // try
  catch (const exc::SQLSelectException &ex) {
    std::cerr << "Server: " << ex.what() << std::endl;
    return false;
  }
}
//
//
//
std::string ServerSession::getUserPasswordSrvSql(const UserLoginPasswordDTO &userLoginPasswordDTO) {
  PGresult *result;

  std::string sql = "";

  try {

    std::string loginEsc = userLoginPasswordDTO.login;
    for (std::size_t pos = 0; (pos = loginEsc.find('\'', pos)) != std::string::npos; pos += 2) {
      loginEsc.replace(pos, 1, "''");
    }

    std::string passwordHashEsc = userLoginPasswordDTO.passwordhash;
    for (std::size_t pos = 0; (pos = passwordHashEsc.find('\'', pos)) != std::string::npos; pos += 2) {
      passwordHashEsc.replace(pos, 1, "''");
    }

    sql = R"(with user_record as (
		select id as user_id 
		from public.users 
		where login = ')";
    sql += loginEsc + "')";

    sql += R"(select password_hash from public.users_passhash as ph join user_record ur on ph
	.user_id = ur.user_id where password_hash = ')";
    sql += passwordHashEsc + "';";

    result = execSQL(this->getPGConnection(), sql);

    if (result == nullptr)
      throw exc::SQLSelectException(", checkUserPasswordSrvSql");

    if (PQresultStatus(result) == PGRES_TUPLES_OK && PQntuples(result) > 0) {

      std::string value = PQgetvalue(result, 0, 0);
      PQclear(result);
      return value;

    } else {
      return "";
    }
  } // try
  catch (const exc::SQLSelectException &ex) {
    std::cerr << "Server: " << ex.what() << std::endl;
    return "";
  }
}

//
//
//
// Database-backed DTO assembly helpers.
std::optional<UserDTO> ServerSession::FillForSendUserDTOFromSrvSQL(const std::string &login,
                                                                   [[maybe_unused]] bool loginUser) {

  PGresult *result = nullptr;

  std::string sql = "";

  try {

    std::string loginEsc = login;
    for (std::size_t pos = 0; (pos = loginEsc.find('\'', pos)) != std::string::npos; pos += 2) {
      loginEsc.replace(pos, 1, "''");
    }

    sql = R"(select * from public.users as us  
		join public.users_passhash as ph on ph.user_id = us.id
		where us.login = ')";
    sql += loginEsc + "';";

    result = execSQL(this->getPGConnection(), sql);

    if (result == nullptr)
      throw exc::SQLSelectException(", FillForSendUserDTOFromSrvSQL");

    if (PQresultStatus(result) == PGRES_TUPLES_OK && PQntuples(result) > 0) {

      UserDTO userDTO;

      userDTO.login = PQgetvalue(result, 0, 1);
      userDTO.userName = PQgetvalue(result, 0, 2);
      userDTO.email = PQgetvalue(result, 0, 3);
      userDTO.phone = PQgetvalue(result, 0, 4);
      userDTO.passwordhash = PQgetvalue(result, 0, 6);

      PQclear(result);
      return userDTO;

    } else {
      PQclear(result);
      throw exc::SQLSelectException(", FillForSendUserDTOFromSrvSQL");
    }
  } // try
  catch (const exc::SQLSelectException &ex) {
    if (result != nullptr)
      PQclear(result);
    std::cerr << "Server: " << ex.what() << std::endl;
    return std::nullopt;
  }
}

std::optional<std::vector<UserDTO>> ServerSession::FillForSendSeveralUsersDTOFromSrvSQL(
  const std::vector<std::string> logins) {

  auto value = getSeveralUsersDTOFromSrvSQL(this->getPGConnection(), logins);

  if (!value.has_value())
    return std::nullopt;

  return value;
}

//
//
//
// Retrieve a single user chat
std::optional<ChatDTO> ServerSession::FillForSendOneChatDTOFromSrvSQL(const std::string &chat_id,
                                                                      const std::string &login) {
  ChatDTO chatDTO;

  // Took chatId and login
  chatDTO.chatId = static_cast<std::size_t>(std::stoull(chat_id));
  chatDTO.senderLogin = login;

  // Retrieve the participant list
  auto participants = getChatParticipantsSQL(this->getPGConnection(), chat_id);

  try {

    if (!participants.has_value()) {
      throw exc::ChatListNotFoundException(login);
    }

    auto deletedMessagesMultiset = getChatMessagesDeletedStatusSQL(this->getPGConnection(), chat_id);

    if (deletedMessagesMultiset.has_value() && deletedMessagesMultiset.value().size() > 0) {

      // Iterate over participants
      for (auto &participant : participants.value()) {

        // Take the specific participant
        const auto &participantLogin = participant.login;

        // Take the array of their values
        const auto &range = deletedMessagesMultiset.value().equal_range({participantLogin, 0});

        // Iterate over all of the user's messages and add them to the outgoing vector
        if (std::distance(range.first, range.second)) {
          for (auto it = range.first; it != range.second; ++it) {

            // Fill deletedMessageIds
            participant.deletedMessageIds.push_back(it->second);
          } // for range

        } // if distance

      } // for participant

    } // if deletedMessagesMultiset

  } // try
  catch (const exc::ChatListNotFoundException &ex) {
    std::cerr << "Server: FillForSendOneChatDTOFromSrvSQL. " << ex.what() << std::endl;
    return std::nullopt;
  }

  chatDTO.participants = participants.value();

  return chatDTO;
}
//
//
// Retrieve all of the user's chats
std::optional<std::vector<ChatDTO>> ServerSession::FillForSendAllChatDTOFromSrvSQL(const std::string &login) {

  // Took the chat list
  auto chatList = getChatListSQL(this->getPGConnection(), login);

  if (chatList.size() == 0)
    return std::nullopt;

  std::vector<ChatDTO> chatDTOResultVector;

  // Iterate over chats in the chat list
  for (const auto &chat : chatList) {

    auto tempDTO = FillForSendOneChatDTOFromSrvSQL(chat, login);

    if (tempDTO.has_value())
      chatDTOResultVector.push_back(tempDTO.value());
    else {
      std::cerr << "Server: FillForSendAllChatDTOFromSrvSQL. Chat_id " << chat << " not populated" << std::endl;
      continue;
    }
  } // first for

  return chatDTOResultVector;
}
//
//
// Retrieve the user's messages for a specific chat
std::optional<MessageChatDTO> ServerSession::fillForSendChatMessageDTOFromSrvSQL(const std::string &chat_id) {

  auto messageChatDTO = getChatMessagesSQL(this->getPGConnection(), chat_id);

  if (!messageChatDTO.has_value())
    return std::nullopt;

  return messageChatDTO.value();
}
//
//
// Retrieve all of the user's messages
std::optional<std::vector<MessageChatDTO>> ServerSession::fillForSendAllMessageDTOFromSrvSQL(const std::string login) {

  std::vector<MessageChatDTO> messageChatDTOVector;

  // Took the chat list
  auto chatList = getChatListSQL(this->getPGConnection(), login);

  if (chatList.size() == 0)
    return std::nullopt;

  // Iterate over chats in the chat list
  for (const auto &chat_id : chatList) {

    const auto &messageChatDTO = fillForSendChatMessageDTOFromSrvSQL(chat_id);

    if (messageChatDTO.has_value())
      messageChatDTOVector.push_back(messageChatDTO.value());
  } // for

  if (messageChatDTOVector.size() == 0)
    return std::nullopt;
  else
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

    // REQUIRED: allow reuse + broadcast
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

    std::cout << "[UDP] Server listening for UDP discovery on port " << listenPort << std::endl;

    while (true) {
      char buffer[128] = {0};
      sockaddr_in clientAddr{};
      socklen_t addrLen = sizeof(clientAddr);

      ssize_t bytesReceived = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0, (sockaddr *)&clientAddr, &addrLen);

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
//
//
// Build and return the user, chats, and messages in response to the request
std::optional<PacketListDTO> ServerSession::registerOnDeviceDataSrvSQL(const std::string login) {

  PacketListDTO packetListDTO;

  //   const auto login = user->getLogin();

  UserDTO userDTO;

  auto const &tempUserDTO = FillForSendUserDTOFromSrvSQL(login, true);

  if (tempUserDTO.has_value())
    userDTO = tempUserDTO.value();
  else
    return std::nullopt;

  // Build the user
  PacketDTO packetDTO;
  packetDTO.requestType = RequestType::RqFrClientRegisterUser;
  packetDTO.structDTOClassType = StructDTOClassType::userDTO;
  packetDTO.reqDirection = RequestDirection::ClientToSrv;
  packetDTO.structDTOPtr = std::make_shared<StructDTOClass<UserDTO>>(userDTO);

  packetListDTO.packets.push_back(packetDTO);

  // Build the chats

  auto chatDTOVector = FillForSendAllChatDTOFromSrvSQL(login);

  if (chatDTOVector.has_value()) {

    for (const auto &pct : chatDTOVector.value()) {
      PacketDTO packetDTO;
      packetDTO.requestType = RequestType::RqFrClientRegisterUser;
      packetDTO.structDTOClassType = StructDTOClassType::chatDTO;
      packetDTO.reqDirection = RequestDirection::ClientToSrv;
      packetDTO.structDTOPtr = std::make_shared<StructDTOClass<ChatDTO>>(pct);

      packetListDTO.packets.push_back(packetDTO);
    }
  }

  // Build the messages

  const auto &messageChatDTO = fillForSendAllMessageDTOFromSrvSQL(login);

  if (messageChatDTO.has_value()) {

    for (const auto &pct : messageChatDTO.value()) {
      PacketDTO packetDTO;
      packetDTO.requestType = RequestType::RqFrClientRegisterUser;
      packetDTO.structDTOClassType = StructDTOClassType::messageChatDTO;
      packetDTO.reqDirection = RequestDirection::ClientToSrv;
      packetDTO.structDTOPtr = std::make_shared<StructDTOClass<MessageChatDTO>>(pct);

      packetListDTO.packets.push_back(packetDTO);
    }
  }

  for (std::size_t i = 0; i < packetListDTO.packets.size(); ++i) {
    std::cerr << "[PACKET " << i << "] type = " << static_cast<int>(packetListDTO.packets[i].structDTOClassType)
              << std::endl;
  }
  return packetListDTO;
}

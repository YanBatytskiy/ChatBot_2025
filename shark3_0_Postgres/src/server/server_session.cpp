#include "server_session.h"
#include "dto/dto_struct.h"
#include "exception/login_exception.h"
#include "exception/network_exception.h"
#include "exception/sql_exception.h"
#include "exception/validation_exception.h"
#include "message/message.h"
#include "sql_server.h"
#include "system/serialize.h"
#include "system/system_function.h"
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
PGconn *ServerSession::getPGConnection() { return _pqConnection; }
const PGconn *ServerSession::getPGConnection() const { return _pqConnection; }

ServerConnectionConfig &ServerSession::getServerConnectionConfig() { return _serverConnectionConfig; }

int &ServerSession::getConnection() { return _connection; }
const int &ServerSession::getConnection() const { return _connection; }

// setters
void ServerSession::setPgConnection(PGconn *connection) { _pqConnection = connection; }

void ServerSession::setConnection(const std::uint8_t &connection) { _connection = connection; }
//
//
// transport

bool ServerSession::isConnected() const { return _connection > 0 && fcntl(_connection, F_GETFD) != -1; }

void ServerSession::runServer(int socketFd) {
  try {

    // если соединение уже есть — проверим его валидность
    if (_connection > 0) {
      // Проверим валидность дескриптора
      if (fcntl(_connection, F_GETFD) != -1) {
        // Соединение уже активно — ничего делать не нужно
        return;
      } else {
        // Соединение есть, но оно невалидно — закрываем
        close(_connection);
        _connection = -1;
      }
    }
    // создаем новое соединение
    struct sockaddr_in client {};
    socklen_t client_len = sizeof(client);

    _connection = accept(socketFd, (struct sockaddr *)&client, &client_len);
    if (_connection < 0) {
      throw exc::ConnectNotAcceptException();
    }

    // сохранили номер сессии
    std::cout << "[Инфо] Новое соединение установлено" << std::endl;

  } catch (const exc::ConnectNotAcceptException &ex) {
    std::cerr << "Сервер. " << ex.what() << std::endl;
  } catch (const std::exception &ex) {
    std::cerr << "Сервер. Неизвестная ошибка. " << ex.what() << std::endl;
  }
}
//
//
//
void ServerSession::listeningClients() {

  try {
    // std::vector<std::uint8_t> buffer(MESSAGE_LENGTH);

    // // проверка
    // // std::cerr << "[DEBUG] buffer.size() = " << buffer.size() << std::endl;

    // if (buffer.empty()) {
    //   std::cerr << "[Ошибка] Буфер пустой (buffer.size() == 0)" << std::endl;
    //   std::exit(1);
    // }

    // if (buffer.data() == nullptr) {
    //   std::cerr << "[Ошибка] buffer.data() == nullptr" << std::endl;
    //   std::exit(1);
    // }

    if (_connection <= 0) {
      throw exc::SocketInvalidException();
    }

    // проверка
    // if (MESSAGE_LENGTH == 0)
    //   throw exc::CreateBufferException();

    if (_connection <= 0 || fcntl(_connection, F_GETFD) == -1) {
      throw exc::SocketInvalidException();
    }
    // проверка

    // std::cerr << "[DEBUG] connection fd = " << connection << std::endl;

    // 1. читаем 4 байта длины
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

    // 2. читаем len байт данных
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

    // берем первый пакет - хедер
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
    std::cerr << "Сервер: " << ex.what() << " = " << _connection << std::endl;
    close(_connection);
    _connection = -1;
    return;
  } catch (const exc::CreateBufferException &ex) {
    std::cerr << "Сервер: " << ex.what() << " = " << MESSAGE_LENGTH << std::endl;
  } catch (const exc::ReceiveDataException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    _connection = -1;
    return;
  } catch (const exc::HeaderWrongTypeException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    _connection = -1;
    return;
  } catch (const exc::HeaderWrongDataException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
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

    // Проверка валидности соединения
    if (connection <= 0 || fcntl(connection, F_GETFD) == -1)
      throw exc::SocketInvalidException();

    // Сериализуем пакеты

    auto PacketSendBinary = serializePacketList(packetListForSend.packets);

    // Проверка: не отправлять пустой буфер
    if (PacketSendBinary.empty())
      throw exc::SendDataException();

    // проверка
    // std::cerr << "[DEBUG] Отправка пакета. Размер: " << PacketSendBinary.size() << " байт" << std::endl;
    // for (std::size_t i = 0; i < PacketSendBinary.size(); ++i) {
    //   std::cerr << static_cast<int>(PacketSendBinary[i]) << " ";
    // }
    // std::cerr << std::endl;

    // Отправка данных
    uint32_t len = htonl(PacketSendBinary.size());

    // std::cerr << "[SERVER DEBUG] requestType of first packet = "
    //           << static_cast<int>(packetListForSend.packets[0].requestType) << std::endl;

    send(connection, &len, 4, 0);

    // std::cerr << "[SERVER DEBUG] requestType of first packet = "
    //           << static_cast<int>(packetListForSend.packets[0].requestType) << std::endl;
    bytesSent = send(connection, PacketSendBinary.data(), PacketSendBinary.size(), 0);

    // Проверка результата
    if (bytesSent <= 0)
      throw exc::SendDataException();

    if (bytesSent != static_cast<ssize_t>(PacketSendBinary.size()))
      throw exc::SendDataException();
  } catch (const exc::SendDataException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    std::cerr << "Send failed. Bytes sent = " << bytesSent << std::endl;
    return false;
  } catch (const exc::NetworkException &ex) {
    std::cerr << "Сервер sendPacketListDTO: " << ex.what() << std::endl;
    return false;
  } catch (const std::exception &ex) {
    std::cerr << "Сервер: Неизвестная ошибка. sendPacketListDTO" << ex.what() << std::endl;
    return false;
  }
  return true;
}
//
//
//
// Request processing

bool ServerSession::routingRequestsFromClient(PacketListDTO &packetListReceived, const RequestType &requestType,
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

  // создали структуру вектора пакетов для отправки
  PacketListDTO packetDTOListForSend;
  packetDTOListForSend.packets.clear();

  switch (requestType) {
  case RequestType::RqFrClientCheckLogin: {

    // отдельный пакет для отправки
    PacketDTO packetDTOForSend;
    packetDTOForSend.requestType = requestType;
    packetDTOForSend.structDTOClassType = StructDTOClassType::responceDTO;
    packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;

    const auto &packet = static_cast<const StructDTOClass<UserLoginDTO> &>(*packetListReceived.packets[0].structDTOPtr)
                             .getStructDTOClass();

    // пакет для отправки
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
    // отдельный пакет для отправки
    PacketDTO packetDTOForSend;
    packetDTOForSend.requestType = requestType;
    packetDTOForSend.structDTOClassType = StructDTOClassType::responceDTO;
    packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;

    const auto &packet = static_cast<const StructDTOClass<UserLoginPasswordDTO> &>(
                             *packetListReceived.packets[0].structDTOPtr)
                             .getStructDTOClass();

    // пакет для отправки
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

    // пакет для отправки
    PacketDTO packetDTOForSend;
    packetDTOForSend.requestType = RequestType::RqFrClientFindUserByPart;
    packetDTOForSend.structDTOClassType = StructDTOClassType::userDTO;
    packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;

    for (const auto& userDTO : userDTOVector.value()) {

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

    // пакет для отправки
    PacketDTO packetDTOForSend;
    packetDTOForSend.requestType = RequestType::RqFrClientSetLastReadMessage;
    packetDTOForSend.structDTOClassType = StructDTOClassType::responceDTO;
    packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;

    if (!value) {
      std::cerr << "Сервер: RqFrClientSetLastReadMessage. Ошибка базы. Значение не установлено." << std::endl;
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
                                            int connection) { // создали структуру вектора пакетов для отправки

  PacketListDTO packetDTOListForSend;
  packetDTOListForSend.packets.clear();

  try {
    switch (requestType) {
    case RequestType::RqFrClientCreateUser: {

      if (packetListReceived.packets.empty() ||
          packetListReceived.packets[0].structDTOClassType != StructDTOClassType::userDTO) {
        throw exc::EmptyPacketException();
      }

      // отдельный пакет для отправки
      PacketDTO packetDTOForSend;
      packetDTOForSend.requestType = requestType;
      packetDTOForSend.structDTOClassType = StructDTOClassType::responceDTO;
      packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;

      const auto &packet = static_cast<const StructDTOClass<UserDTO> &>(*packetListReceived.packets[0].structDTOPtr)
                               .getStructDTOClass();

      // пакет для отправки
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
      // отдельный пакет для отправки
      PacketDTO packetDTOForSend;
      packetDTOForSend.requestType = requestType;
      packetDTOForSend.structDTOClassType = StructDTOClassType::responceDTO;
      packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;

      // берем чат
      const auto &packetChat = static_cast<const StructDTOClass<ChatDTO> &>(*packetListReceived.packets[0].structDTOPtr)
                                   .getStructDTOClass();

      // берем cообщение
      auto &packetMessage = static_cast<StructDTOClass<MessageChatDTO> &>(*packetListReceived.packets[1].structDTOPtr)
                                .getStructDTOClass();

      // пакет для отправки
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

      // отдельный пакет для отправки
      PacketDTO packetDTOForSend;
      packetDTOForSend.requestType = requestType;
      packetDTOForSend.structDTOClassType = StructDTOClassType::responceDTO;
      packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;

      // пакет для отправки
      ResponceDTO responceDTO;
      responceDTO.anyNumber = 0;

      // доcтаем пакерт
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
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return false;
  } catch (const exc::NetworkException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return false;
  } catch (const std::bad_cast &ex) {
    std::cerr << "Сервер: Неправильный тип пакета. " << ex.what() << std::endl;
    return false;
  }
  return true;
}
//
//
//
bool ServerSession::processingGetUserData(PacketListDTO &packetListReceived, const RequestType &requestType,
                                          int connection) {

  // вектор логинов для запроса в базе
  std::vector<std::string> logins;
  logins.clear();

  // создали структуру вектора пакетов для отправки
  PacketListDTO packetDTOListForSend;

  try {
    // заполняем вектор логинов для запроса у базы
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

    // получаем массив пользователей
    if (logins.size() == 0)
      throw exc::UserNotFoundException();

    auto userDTOVector = FillForSendSeveralUsersDTOFromSrvSQL(logins);

    if (!userDTOVector.has_value())
      throw exc::UserNotFoundException();

    for (const auto& userDTO : userDTOVector.value()) {

      // формируем пользователя
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
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return false;
  } catch (const exc::HeaderWrongTypeException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return false;
  } catch (const exc::UserNotFoundException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
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
    std::cerr << "Сервер: " << ex.what() << std::endl;
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
    std::cerr << "Сервер: " << ex.what() << std::endl;
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
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return "";
  }
}

//
//
//
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!11
std::optional<UserDTO> ServerSession::FillForSendUserDTOFromSrvSQL(const std::string &login, bool loginUser) {

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
    std::cerr << "Сервер: " << ex.what() << std::endl;
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
// получаем один чат пользователя
std::optional<ChatDTO> ServerSession::FillForSendOneChatDTOFromSrvSQL(const std::string &chat_id,
                                                                      const std::string &login) {
  ChatDTO chatDTO;

  // взяли chatId  и логин
  chatDTO.chatId = static_cast<std::size_t>(std::stoull(chat_id));
  chatDTO.senderLogin = login;

  // получаем список участников
  auto participants = getChatParticipantsSQL(this->getPGConnection(), chat_id);

  try {

    if (!participants.has_value()) {
      throw exc::ChatListNotFoundException(login);
    }

    auto deletedMessagesMultiset = getChatMessagesDeletedStatusSQL(this->getPGConnection(), chat_id);

    if (deletedMessagesMultiset.has_value() && deletedMessagesMultiset.value().size() > 0) {

      // перебираем участников
      for (auto &participant : participants.value()) {

        // берем конкретного участника
        const auto &participantLogin = participant.login;

        // берем массив его значений
        const auto &range = deletedMessagesMultiset.value().equal_range({participantLogin, 0});

        // перебираем все сообщения пользователя и добавляем его в вектор для отправки
        if (std::distance(range.first, range.second)) {
          for (auto it = range.first; it != range.second; ++it) {

            // заполняем deletedMessageIds
            participant.deletedMessageIds.push_back(it->second);
          } // for range

        } // if distance

      } // for participant

    } // if deletedMessagesMultiset

  } // try
  catch (const exc::ChatListNotFoundException &ex) {
    std::cerr << "Сервер: FillForSendOneChatDTOFromSrvSQL. " << ex.what() << std::endl;
    return std::nullopt;
  }

  chatDTO.participants = participants.value();

  return chatDTO;
}
//
//
// получаем все чаты пользователя
std::optional<std::vector<ChatDTO>> ServerSession::FillForSendAllChatDTOFromSrvSQL(const std::string &login) {

  // взяли чат лист
  auto chatList = getChatListSQL(this->getPGConnection(), login);

  if (chatList.size() == 0)
    return std::nullopt;

  std::vector<ChatDTO> chatDTOResultVector;

  // перебираем чаты в чат листе
  for (const auto &chat : chatList) {

    auto tempDTO = FillForSendOneChatDTOFromSrvSQL(chat, login);

    if (tempDTO.has_value())
      chatDTOResultVector.push_back(tempDTO.value());
    else {
      std::cerr << "Сервер: FillForSendAllChatDTOFromSrvSQL. Chat_id " << chat << " не заполнен" << std::endl;
      continue;
    }
  } // first for

  return chatDTOResultVector;
}
//
//
// получаем сообщения пользователя конкретного чата
std::optional<MessageChatDTO> ServerSession::fillForSendChatMessageDTOFromSrvSQL(const std::string &chat_id) {

  auto messageChatDTO = getChatMessagesSQL(this->getPGConnection(), chat_id);

  if (!messageChatDTO.has_value())
    return std::nullopt;

  return messageChatDTO.value();
}
//
//
// получаем все сообщения пользователя
std::optional<std::vector<MessageChatDTO>> ServerSession::fillForSendAllMessageDTOFromSrvSQL(const std::string login) {

  std::vector<MessageChatDTO> messageChatDTOVector;

  // взяли чат лист
  auto chatList = getChatListSQL(this->getPGConnection(), login);

  if (chatList.size() == 0)
    return std::nullopt;

  // перебираем чаты в чат листе
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

  std::cout << "[DEBUG] runUDPServerDiscovery стартовал" << std::endl;

  try {
    if (udpSocket < 0)
      throw exc::CreateSocketTypeException();

    // ОБЯЗАТЕЛЬНО: разрешить reuse + broadcast
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

    std::cout << "[UDP] Сервер слушает UDP discovery на порту " << listenPort << std::endl;

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
// формируем и отдаем в ответ на запрос юзера, чаты и сообщения
std::optional<PacketListDTO> ServerSession::registerOnDeviceDataSrvSQL(const std::string login) {

  PacketListDTO packetListDTO;

  //   const auto login = user->getLogin();

  UserDTO userDTO;

  auto const &tempUserDTO = FillForSendUserDTOFromSrvSQL(login, true);

  if (tempUserDTO.has_value())
    userDTO = tempUserDTO.value();
  else
    return std::nullopt;

  // формируем пользователя
  PacketDTO packetDTO;
  packetDTO.requestType = RequestType::RqFrClientRegisterUser;
  packetDTO.structDTOClassType = StructDTOClassType::userDTO;
  packetDTO.reqDirection = RequestDirection::ClientToSrv;
  packetDTO.structDTOPtr = std::make_shared<StructDTOClass<UserDTO>>(userDTO);

  packetListDTO.packets.push_back(packetDTO);

  // формируем чаты

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

  // формируем сообщения

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
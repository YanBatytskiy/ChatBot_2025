#pragma once
#include "my_exception.h"

namespace exc {

class NetworkException : public MyException {
public:
  explicit NetworkException(const std::string &message) : MyException("Network Exception: " + message){};

  /**
   * @brief Конструктор, оборачивающий std::exception.
   */
  explicit NetworkException(const std::exception &e)
      : MyException(std::string("Network Exception (wrapped): ") + e.what()) {}

  /**
   * @brief Конструктор, оборачивающий неизвестное исключение.
   */
  NetworkException() : MyException("Network Exception: неизвестное исключение.") {}
};

// ошибки соединения

class CreateSocketTypeException : public NetworkException {
public:
  CreateSocketTypeException() : NetworkException("Создание сокета не удалось."){};
};

class CreateBufferException : public NetworkException {
public:
  CreateBufferException() : NetworkException("Ошибка создания буфера."){};
};

class SocketInvalidException : public NetworkException {
public:
  SocketInvalidException() : NetworkException("Сокет non valid."){};
};

class ServerFindLANException : public NetworkException {
public:
  ServerFindLANException() : NetworkException("Сервер в локальной сети не найден."){};
};
class ConnectionToServerException : public NetworkException {
public:
  ConnectionToServerException() : NetworkException("Не удалось подключиться к Серверу."){};
};
class LostConnectionException : public NetworkException {
public:
  LostConnectionException() : NetworkException("Соединение с сервером потеряно."){};
};

class ConnectNotAcceptException : public NetworkException {
public:
  ConnectNotAcceptException() : NetworkException("Серверу не удалось принять соединение."){};
};

// Send And Receive Exception
class SendDataException : public NetworkException {
public:
  SendDataException() : NetworkException("Ошибка при отправке данных."){};
};
class ReceiveDataException : public NetworkException {
public:
  ReceiveDataException() : NetworkException("Ошибка при получении данных."){};
};
class WrongPacketSizeException : public NetworkException {
public:
  WrongPacketSizeException() : NetworkException("Ошибка неправильный размер пакета."){};
};

class EmptyPacketException : public NetworkException {
public:
  EmptyPacketException() : NetworkException("Входящий пакет пустой."){};
};
class HeaderWrongTypeException : public NetworkException {
public:
  HeaderWrongTypeException() : NetworkException("Хедер-пакет имеет неправильный тип."){};
};

class HeaderWrongDataException : public NetworkException {
public:
  HeaderWrongDataException() : NetworkException("Хедер-пакет имеет неправильные данные."){};
};

// ошибки сериализации
class UnsupportedSirializeTypeException : public NetworkException {
public:
  UnsupportedSirializeTypeException() : NetworkException("Неподдерживаемый тип данных сериализации."){};
};

class UnsupportedDeSirializeTypeException : public NetworkException {
public:
  UnsupportedDeSirializeTypeException() : NetworkException("Неподдерживаемый тип данных десериализации."){};
};

// ошибки создавния объектов

class CreateChatException : public NetworkException {
public:
  CreateChatException() : NetworkException("Ошибка создания нового чата."){};
};
class CreateChatIdException : public NetworkException {
public:
  CreateChatIdException() : NetworkException("Ошибка создания Id для нового чата."){};
};
class CreateMessageException : public NetworkException {
public:
  CreateMessageException() : NetworkException("Ошибка создания нового сообщения."){};
};
class CreateMessageIdException : public NetworkException {
public:
  CreateMessageIdException() : NetworkException("Ошибка создания Id для нового сообщения."){};
};

class CreateUserException : public NetworkException {
public:
  CreateUserException() : NetworkException("Ошибка создания нового пользователя."){};
};
// прочее

class InternalDataErrorException : public NetworkException {
public:
  InternalDataErrorException() : NetworkException("Ошибка обработки пакета DTO "){};
};

class WrongresponceTypeException : public NetworkException {
public:
  WrongresponceTypeException() : NetworkException("Неправильный тип полученного пакета."){};
};
class LastReadMessageException : public NetworkException {
public:
  LastReadMessageException() : NetworkException("Ошибка синхронизации последнего прочитанного сообщения."){};
};

} // namespace exc
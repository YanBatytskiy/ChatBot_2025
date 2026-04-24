#pragma once
#include "my_exception.h"
#include <exception>
#include <string>

namespace exc {

class NetworkException : public MyException {
public:
  explicit NetworkException(const std::string &message) : MyException("Network Exception: " + message) {};

  /**
   * @brief Constructor that wraps std::exception.
   */
  explicit NetworkException(const std::exception &e)
      : MyException(std::string("Network Exception (wrapped): ") + e.what()) {}

  /**
   * @brief Constructor that wraps an unknown exception.
   */
  NetworkException() : MyException("Network Exception: unknown exception.") {}
};

// connection errors

class CreateSocketTypeException : public NetworkException {
public:
  CreateSocketTypeException() : NetworkException("Socket creation failed.") {};
};

class CreateBufferException : public NetworkException {
public:
  CreateBufferException() : NetworkException("Buffer creation error.") {};
};

class SocketInvalidException : public NetworkException {
public:
  SocketInvalidException() : NetworkException("Socket non valid.") {};
};

class ServerFindLANException : public NetworkException {
public:
  ServerFindLANException() : NetworkException("Server not found on the LAN.") {};
};
class ConnectionToServerException : public NetworkException {
public:
  ConnectionToServerException() : NetworkException("Failed to connect to the server.") {};
};
class LostConnectionException : public NetworkException {
public:
  LostConnectionException() : NetworkException("Connection to the server is lost.") {};
};

class ConnectNotAcceptException : public NetworkException {
public:
  ConnectNotAcceptException() : NetworkException("Server failed to accept the connection.") {};
};

// Send And Receive Exception
class SendDataException : public NetworkException {
public:
  SendDataException() : NetworkException("Error sending data.") {};
};
class ReceiveDataException : public NetworkException {
public:
  ReceiveDataException() : NetworkException("Error receiving data.") {};
};
class WrongPacketSizeException : public NetworkException {
public:
  WrongPacketSizeException() : NetworkException("Error: wrong packet size.") {};
};

class EmptyPacketException : public NetworkException {
public:
  EmptyPacketException() : NetworkException("Incoming packet is empty.") {};
};
class HeaderWrongTypeException : public NetworkException {
public:
  HeaderWrongTypeException() : NetworkException("Header packet has the wrong type.") {};
};

class HeaderWrongDataException : public NetworkException {
public:
  HeaderWrongDataException() : NetworkException("Header packet has invalid data.") {};
};

// serialization errors
class UnsupportedSirializeTypeException : public NetworkException {
public:
  UnsupportedSirializeTypeException() : NetworkException("Unsupported data type for serialization.") {};
};

class UnsupportedDeSirializeTypeException : public NetworkException {
public:
  UnsupportedDeSirializeTypeException() : NetworkException("Unsupported data type for deserialization.") {};
};

// object-creation errors

class CreateChatException : public NetworkException {
public:
  CreateChatException() : NetworkException("Error creating a new chat.") {};
};
class CreateChatIdException : public NetworkException {
public:
  CreateChatIdException() : NetworkException("Error creating an Id for the new chat.") {};
};
class CreateMessageException : public NetworkException {
public:
  CreateMessageException() : NetworkException("Error creating a new message.") {};
};
class CreateMessageIdException : public NetworkException {
public:
  CreateMessageIdException() : NetworkException("Error creating an Id for the new message.") {};
};

class CreateUserException : public NetworkException {
public:
  CreateUserException() : NetworkException("Error creating a new user.") {};
};
// other

class InternalDataErrorException : public NetworkException {
public:
  InternalDataErrorException() : NetworkException("DTO packet processing error ") {};
};

class WrongresponceTypeException : public NetworkException {
public:
  WrongresponceTypeException() : NetworkException("Wrong response packet type.") {};
};
class LastReadMessageException : public NetworkException {
public:
  LastReadMessageException() : NetworkException("Synchronization error for the last read message.") {};
};

} // namespace exc

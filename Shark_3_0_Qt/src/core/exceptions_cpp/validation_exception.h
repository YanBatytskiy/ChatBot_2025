#pragma once
#include "my_exception.h"
#include <string>
namespace exc {

/**
 * @class ValidationException
 * @brief Base exception class for validation-related errors.
 * @details Inherits from MyException and prepends "Validation Exception: " to the provided message.
 */
class ValidationException : public MyException {
public:
  /**
   * @brief Constructor for ValidationException.
   * @param message The error message describing the validation issue.
   */
  explicit ValidationException(const std::string &message) : MyException("Validation Exception: " + message) {};
};

/**
 * @class EmptyInputException
 * @brief Exception thrown when input is empty.
 * @details Inherits from ValidationException with a specific error message.
 */
class EmptyInputException : public ValidationException {
public:
  /**
   * @brief Default constructor for EmptyInputException.
   * @details Sets the error message to "!!!You did not enter anything.".
   */
  EmptyInputException() : ValidationException("!!!You did not enter anything.") {};
};

/**
 * @class InvalidCharacterException
 * @brief Exception thrown when an invalid character is detected.
 * @details Inherits from ValidationException and includes the invalid character in the message.
 */
class InvalidCharacterException : public ValidationException {
public:
  /**
   * @brief Constructor for InvalidCharacterException.
   * @param ch The invalid character.
   * @details Sets the error message to include the invalid character.
   */
  InvalidCharacterException(const std::string &ch) : ValidationException("!!!Invalid character '" + ch + "'") {};
};

/**
 * @class IndexOutOfRangeException
 * @brief Exception thrown when input is out of the valid range.
 * @details Inherits from ValidationException and supports string or char input in the message.
 */
class IndexOutOfRangeException : public ValidationException {
public:
  /**
   * @brief Constructor for IndexOutOfRangeException (string input).
   * @param st The input string that is out of range.
   */
  IndexOutOfRangeException(const std::string &st)
      : ValidationException(std::string("!!!Input '" + st + "' is out of the allowed range.")) {};
  /**
   * @brief Constructor for IndexOutOfRangeException (char input).
   * @param st The input char that is out of range.
   */
  IndexOutOfRangeException(const char &st)
      : ValidationException(std::string("!!!Input '") + st + "' is out of the allowed range.") {};
};

/**
 * @class InvalidQuantityCharacterException
 * @brief Exception thrown when the number of characters is incorrect.
 * @details Inherits from ValidationException with a specific error message.
 */
class InvalidQuantityCharacterException : public ValidationException {
public:
  /**
   * @brief Default constructor for InvalidQuantityCharacterException.
   * @details Sets the error message to "!!!Incorrect number of characters.".
   */
  InvalidQuantityCharacterException() : ValidationException("!!!Incorrect number of characters.") {};
};

/**
 * @class NonCapitalCharacterException
 * @brief Exception thrown when a capital letter is required but not provided.
 * @details Inherits from ValidationException with a specific error message.
 */
class NonCapitalCharacterException : public ValidationException {
public:
  /**
   * @brief Default constructor for NonCapitalCharacterException.
   * @details Sets the error message to "!!!A capital letter is required.".
   */
  NonCapitalCharacterException() : ValidationException("!!!A capital letter is required.") {};
};

/**
 * @class NonDigitalCharacterException
 * @brief Exception thrown when a digit is required but not provided.
 * @details Inherits from ValidationException with a specific error message.
 */
class NonDigitalCharacterException : public ValidationException {
public:
  /**
   * @brief Default constructor for NonDigitalCharacterException.
   * @details Sets the error message to "!!!A digit is required.".
   */
  NonDigitalCharacterException() : ValidationException("!!!A digit is required.") {};
};

/**
 * @class ChatNotFoundException
 * @brief Exception thrown when a chat is not found.
 * @details Inherits from ValidationException with a specific error message.
 */
class ChatNotFoundException : public ValidationException {
public:
  /**
   * @brief Default constructor for ChatNotFoundException.
   * @details Sets the error message to "!!!Error. Chat not found. Contact the administrator".
   */
  ChatNotFoundException() : ValidationException("!!!Error. Chat not found. Contact the administrator") {};
};

class MessagesNotFoundException : public ValidationException {
public:
  MessagesNotFoundException() : ValidationException("!!!Error. No messages. Contact the administrator") {};
};

/**
 * @class UnknownException
 * @brief Exception thrown for unknown errors.
 * @details Inherits from ValidationException with a generic error message.
 */
class UnknownException : public ValidationException {
public:
  /**
   * @brief Constructor for UnknownException.
   * @param str Additional information about the error.
   * @details Sets the error message to "!!!Unknown error. ".
   */
  UnknownException(const std::string &str) : ValidationException("!!!Unknown error. ") {};
};

/**
 * @class BadWeakException
 * @brief Exception thrown when a weak_ptr cannot be locked.
 * @details Inherits from ValidationException with a specific error message.
 */
class BadWeakException : public ValidationException {
public:
  /**
   * @brief Constructor for BadWeakException.
   * @param str Additional information about the error.
   * @details Sets the error message to "!!!weak_ptr lost. ".
   */
  BadWeakException(const std::string &str) : ValidationException("!!!weak_ptr lost. ") {};
};

/**
 * @class UserNotInListException
 * @brief Exception thrown when a user is not found in the participants list.
 * @details Inherits from ValidationException with a specific error message.
 */
class UserNotInListException : public ValidationException {
public:
  /**
   * @brief Default constructor for UserNotInListException.
   * @details Sets the error message to "!!!User is not among the chat participants.".
   */
  UserNotInListException() : ValidationException("!!!User is not among the chat participants.") {};
};

class ChatListNotFoundException : public ValidationException {
public:
  ChatListNotFoundException(const std::string &userLogin)
      : ValidationException("!! Chat list not found for the user with login: " + userLogin) {};
};
} // namespace exc
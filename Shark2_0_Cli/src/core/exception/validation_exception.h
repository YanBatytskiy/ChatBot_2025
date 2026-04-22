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
  explicit ValidationException(const std::string &message) : MyException("Validation Exception: " + message){};
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
   * @details Sets the error message to "!!!Вы ничего не ввели.".
   */
  EmptyInputException() : ValidationException("!!!Вы ничего не ввели."){};
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
  InvalidCharacterException(const std::string &ch) : ValidationException("!!!Недопустимый символ '" + ch + "'"){};
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
      : ValidationException(std::string("!!!Ввод '" + st + "' вне допустимого диапазона.")){};
  /**
   * @brief Constructor for IndexOutOfRangeException (char input).
   * @param st The input char that is out of range.
   */
  IndexOutOfRangeException(const char &st)
      : ValidationException(std::string("!!!Ввод '") + st + "' вне допустимого диапазона."){};
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
   * @details Sets the error message to "!!!Некорректное количество символов.".
   */
  InvalidQuantityCharacterException() : ValidationException("!!!Некорректное количество символов."){};
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
   * @details Sets the error message to "!!!Требуется заглавная буква.".
   */
  NonCapitalCharacterException() : ValidationException("!!!Требуется заглавная буква."){};
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
   * @details Sets the error message to "!!!Требуется цифра.".
   */
  NonDigitalCharacterException() : ValidationException("!!!Требуется цифра."){};
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
   * @details Sets the error message to "!!!Ошибка. Чат не найден. Обратитесь к администратору".
   */
  ChatNotFoundException() : ValidationException("!!!Ошибка. Чат не найден. Обратитесь к администратору"){};
};

class MessagesNotFoundException : public ValidationException {
public:
  MessagesNotFoundException() : ValidationException("!!!Ошибка. Нет сообшений. Обратитесь к администратору"){};
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
   * @details Sets the error message to "!!!Неизвестная ошибка. ".
   */
  UnknownException(const std::string &str) : ValidationException("!!!Неизвестная ошибка. "){};
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
   * @details Sets the error message to "!!!Потеря weak_ptr. ".
   */
  BadWeakException(const std::string &str) : ValidationException("!!!Потеря weak_ptr. "){};
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
   * @details Sets the error message to "!!!Пользователя нет среди участников чата.".
   */
  UserNotInListException() : ValidationException("!!!Пользователя нет среди участников чата."){};
};

class ChatListNotFoundException : public ValidationException {
public:
  ChatListNotFoundException(const std::string &userLogin)
      : ValidationException("!! Чат Лист не найден у пользователя с логином: " + userLogin){};
};
} // namespace exc
#pragma once
#include "validation_exception.h"
namespace exc {

/**
 * @class LoginException
 * @brief Base exception class for login-related errors.
 * @details Inherits from ValidationException and prepends "Sign In Exception: " to the provided message.
 */
class LoginException : public ValidationException {
public:
  /**
   * @brief Constructor for LoginException.
   * @param message The error message describing the login issue.
   */
  explicit LoginException(const std::string &message) : ValidationException("Sign In Exception: " + message){};
};

/**
 * @class UserNotFoundException
 * @brief Exception thrown when a user is not found during login.
 * @details Inherits from LoginException and provides a specific error message.
 */
class UserNotFoundException : public LoginException {
public:
  /**
   * @brief Default constructor for UserNotFoundException.
   * @details Sets the error message to "!!!Такой пользователь не найден. ".
   */
  UserNotFoundException() : LoginException("!!!Такой пользователь не найден. "){};
};

/**
 * @class IncorrectPasswordException
 * @brief Exception thrown when an incorrect password is provided during login.
 * @details Inherits from LoginException and provides a specific error message.
 */
class IncorrectPasswordException : public LoginException {
public:
  /**
   * @brief Default constructor for IncorrectPasswordException.
   * @details Sets the error message to "!!!Неверный пароль.".
   */
  IncorrectPasswordException() : LoginException("!!!Неверный пароль."){};
};
}
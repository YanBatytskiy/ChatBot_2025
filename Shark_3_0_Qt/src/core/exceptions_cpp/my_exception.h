#pragma once
#include <string>
namespace exc {

/**
 * @class MyException
 * @brief Custom exception class for handling errors.
 * @details Inherits from std::exception and stores a custom error message.
 */
class MyException : public std::exception {
private:
  std::string _message; ///< The error message.

public:
  /**
   * @brief Constructor for MyException.
   * @param message The error message to be stored.
   */
  explicit MyException(const std::string &message) : _message(message) {};

  /**
   * @brief Returns the error message.
   * @return A C-string representation of the error message.
   * @details Overrides the what() method from std::exception.
   */
  const char *what() const noexcept override { return _message.c_str(); };
};
}
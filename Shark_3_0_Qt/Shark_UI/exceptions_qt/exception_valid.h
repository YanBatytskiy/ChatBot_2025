#ifndef EXCEPTION_VALID_H
#define EXCEPTION_VALID_H
#include "errorbus.h"

namespace exc_qt {

class ValidationException : public MyException {
public:
  explicit ValidationException(const std::string &message) : MyException("Network exception: " + message) {};
};

class InvalidCharacterException : public ValidationException {
public:
  InvalidCharacterException(const std::string &ch) : ValidationException("!!!Invalid character '" + ch + "'") {};
};

class IndexOutOfRangeException : public ValidationException {
public:
  IndexOutOfRangeException(const std::string &st)
      : ValidationException(std::string("!!!Input '" + st + "' is out of the allowed range.")) {};
  IndexOutOfRangeException(const char &st)
      : ValidationException(std::string("!!!Input '") + st + "' is out of the allowed range.") {};
};

class InvalidQuantityCharacterException : public ValidationException {
public:
  InvalidQuantityCharacterException() : ValidationException("!!!Incorrect number of characters.") {};
};

class NonCapitalCharacterException : public ValidationException {
public:
  NonCapitalCharacterException() : ValidationException("!!!A capital letter is required.") {};
};

class NonDigitalCharacterException : public ValidationException {
public:
  NonDigitalCharacterException() : ValidationException("!!!A digit is required.") {};
};

class UnknownException : public ValidationException {
public:
  UnknownException(const std::string &str) : ValidationException("!!!Unknown error. ") {};
};

} // namespace exc_qt
#endif // EXCEPTION_VALID_H

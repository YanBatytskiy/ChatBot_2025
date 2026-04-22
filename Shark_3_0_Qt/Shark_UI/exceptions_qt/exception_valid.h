#ifndef EXCEPTION_VALID_H
#define EXCEPTION_VALID_H
#include "errorbus.h"

namespace exc_qt {


class ValidationException : public MyException{
public:
  explicit ValidationException(const std::string &message) : MyException("Network exception: "+ message) {};
};

class InvalidCharacterException : public ValidationException {
public:
  InvalidCharacterException(const std::string &ch) : ValidationException("!!!Недопустимый символ '" + ch + "'"){};
};

class IndexOutOfRangeException : public ValidationException {
public:
  IndexOutOfRangeException(const std::string &st)
      : ValidationException(std::string("!!!Ввод '" + st + "' вне допустимого диапазона.")){};
  IndexOutOfRangeException(const char &st)
      : ValidationException(std::string("!!!Ввод '") + st + "' вне допустимого диапазона."){};
};

class InvalidQuantityCharacterException : public ValidationException {
public:
  InvalidQuantityCharacterException() : ValidationException("!!!Некорректное количество символов."){};
};

class NonCapitalCharacterException : public ValidationException {
public:
  NonCapitalCharacterException() : ValidationException("!!!Требуется заглавная буква."){};
};

class NonDigitalCharacterException : public ValidationException {
public:
  NonDigitalCharacterException() : ValidationException("!!!Требуется цифра."){};
};

class UnknownException : public ValidationException {
public:
  UnknownException(const std::string &str) : ValidationException("!!!Неизвестная ошибка. "){};
};

}
#endif // EXCEPTION_VALID_H

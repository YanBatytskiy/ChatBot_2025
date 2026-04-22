#pragma once
#include "my_exception.h"

namespace exc {

class sqlException : public MyException {
public:
  explicit sqlException(const std::string &message) : MyException("SQL Exception: " + message){};

  /**
   * @brief Конструктор, оборачивающий std::exception.
   */
  explicit sqlException(const std::exception &e) : MyException(std::string("SQL Exception (wrapped): ") + e.what()) {}

  /**
   * @brief Конструктор, оборачивающий неизвестное исключение.
   */
  sqlException() : MyException("SQL Exception: неизвестное исключение.") {}
};

class SQLExecException : public sqlException {
public:
  SQLExecException(const std::string &message)
      : sqlException("SQL Exception: Ошибка обращения к базе ExecSQL." + message){};
};

class SQLSelectException : public sqlException {
public:
  SQLSelectException(const std::string &message)
      : sqlException("SQL Exception: Ошибка выполнения SELECT: " + message){};
};

class SQLInsertUserException : public sqlException {
public:
  SQLInsertUserException(const std::string &message)
      : sqlException("SQL Exception: Ошибка выполнения INSERTUSER: " + message){};
};

class SQLEmptyBaseException : public sqlException {
public:
  SQLEmptyBaseException(const std::string &message)
      : sqlException("SQL Exception: Ошибка выполнения checkEmptyBase: " + message){};
};

class SQLTableAbscentException : public sqlException {
public:
  SQLTableAbscentException(const std::string &message)
      : sqlException("SQL Exception: Ошибка выполнения наличия таблицы: " + message){};
};

class SQLCreateTableException : public sqlException {
public:
  SQLCreateTableException(const std::string &message)
      : sqlException("SQL Exception: Ошибка выполнения создания таблицы: " + message){};
};

class SQLReadConfigException : public sqlException {
public:
  SQLReadConfigException(const std::string &message)
      : sqlException("SQL Exception: Ошибка чтения файла конфигурации: " + message){};
};
} // namespace exc
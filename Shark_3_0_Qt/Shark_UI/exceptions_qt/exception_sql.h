#ifndef EXCEPTION_SQL_H
#define EXCEPTION_SQL_H
#include "errorbus.h"

namespace exc_qt {


class SQLException : public MyException{
public:
  explicit SQLException(const std::string &message) : MyException("Network exception: "+ message) {};
  explicit SQLException(const std::exception &e) : MyException(std::string("SQL Exception (wrapped): ") + e.what()) {}
  SQLException() : MyException("SQL Exception: неизвестное исключение.") {}
};

class SQLExecException : public SQLException {
public:
  SQLExecException(const std::string &message)
      : SQLException("SQL Exception: Ошибка обращения к базе ExecSQL." + message){};
};

class SQLSelectException : public SQLException {
public:
  SQLSelectException(const std::string &message)
      : SQLException("SQL Exception: Ошибка выполнения SELECT: " + message){};
};

class SQLInsertUserException : public SQLException {
public:
  SQLInsertUserException(const std::string &message)
      : SQLException("SQL Exception: Ошибка выполнения INSERTUSER: " + message){};
};

class SQLEmptyBaseException : public SQLException {
public:
  SQLEmptyBaseException(const std::string &message)
      : SQLException("SQL Exception: Ошибка выполнения checkEmptyBase: " + message){};
};

class SQLTableAbscentException : public SQLException {
public:
  SQLTableAbscentException(const std::string &message)
      : SQLException("SQL Exception: Ошибка выполнения наличия таблицы: " + message){};
};

class SQLCreateTableException : public SQLException {
public:
  SQLCreateTableException(const std::string &message)
      : SQLException("SQL Exception: Ошибка выполнения создания таблицы: " + message){};
};

class SQLReadConfigException : public SQLException {
public:
  SQLReadConfigException(const std::string &message)
      : SQLException("SQL Exception: Ошибка чтения файла конфигурации: " + message){};
};
} // namespace exc
#endif // EXCEPTION_SQL_H

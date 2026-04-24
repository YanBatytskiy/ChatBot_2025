#ifndef EXCEPTION_SQL_H
#define EXCEPTION_SQL_H
#include "errorbus.h"

namespace exc_qt {

class SQLException : public MyException {
public:
  explicit SQLException(const std::string &message) : MyException("Network exception: " + message) {};
  explicit SQLException(const std::exception &e) : MyException(std::string("SQL Exception (wrapped): ") + e.what()) {}
  SQLException() : MyException("SQL Exception: unknown exception.") {}
};

class SQLExecException : public SQLException {
public:
  SQLExecException(const std::string &message)
      : SQLException("SQL Exception: Error accessing the database ExecSQL." + message) {};
};

class SQLSelectException : public SQLException {
public:
  SQLSelectException(const std::string &message)
      : SQLException("SQL Exception: SELECT execution error: " + message) {};
};

class SQLInsertUserException : public SQLException {
public:
  SQLInsertUserException(const std::string &message)
      : SQLException("SQL Exception: INSERTUSER execution error: " + message) {};
};

class SQLEmptyBaseException : public SQLException {
public:
  SQLEmptyBaseException(const std::string &message)
      : SQLException("SQL Exception: checkEmptyBase execution error: " + message) {};
};

class SQLTableAbscentException : public SQLException {
public:
  SQLTableAbscentException(const std::string &message)
      : SQLException("SQL Exception: Error checking table presence: " + message) {};
};

class SQLCreateTableException : public SQLException {
public:
  SQLCreateTableException(const std::string &message)
      : SQLException("SQL Exception: Error creating the table: " + message) {};
};

class SQLReadConfigException : public SQLException {
public:
  SQLReadConfigException(const std::string &message)
      : SQLException("SQL Exception: Error reading the configuration file: " + message) {};
};
} // namespace exc_qt
#endif // EXCEPTION_SQL_H

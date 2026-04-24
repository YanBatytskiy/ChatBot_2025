#pragma once
#include "my_exception.h"

namespace exc {

class sqlException : public MyException {
public:
  explicit sqlException(const std::string &message) : MyException("SQL Exception: " + message) {};

  /**
   * @brief Constructor wrapping a std::exception.
   */
  explicit sqlException(const std::exception &e) : MyException(std::string("SQL Exception (wrapped): ") + e.what()) {}

  /**
   * @brief Constructor wrapping an unknown exception.
   */
  sqlException() : MyException("SQL Exception: unknown exception.") {}
};

class SQLExecException : public sqlException {
public:
  SQLExecException(const std::string &message)
      : sqlException("SQL Exception: Error accessing the database ExecSQL." + message) {};
};

class SQLSelectException : public sqlException {
public:
  SQLSelectException(const std::string &message)
      : sqlException("SQL Exception: SELECT execution error: " + message) {};
};

class SQLInsertUserException : public sqlException {
public:
  SQLInsertUserException(const std::string &message)
      : sqlException("SQL Exception: INSERTUSER execution error: " + message) {};
};

class SQLEmptyBaseException : public sqlException {
public:
  SQLEmptyBaseException(const std::string &message)
      : sqlException("SQL Exception: checkEmptyBase execution error: " + message) {};
};

class SQLTableAbscentException : public sqlException {
public:
  SQLTableAbscentException(const std::string &message)
      : sqlException("SQL Exception: Error checking table presence: " + message) {};
};

class SQLCreateTableException : public sqlException {
public:
  SQLCreateTableException(const std::string &message)
      : sqlException("SQL Exception: Error creating the table: " + message) {};
};

class SQLReadConfigException : public sqlException {
public:
  SQLReadConfigException(const std::string &message)
      : sqlException("SQL Exception: Error reading the configuration file: " + message) {};
};
} // namespace exc

#include "postgres_db.h"
#include "libpq-fe.h"
#include <iostream>

PostgressDatabase::~PostgressDatabase() {
  if (conn_) {
    PQfinish(conn_);
    std::cout << "[DB INFO] Connection closed\n";
  }
}

bool PostgressDatabase::isConnected() const { return conn_ && PQstatus(conn_) == CONNECTION_OK; }

PGconn *PostgressDatabase::getConnection() const { return conn_; }

const std::string PostgressDatabase::getHost() const { return _host; }
const int PostgressDatabase::getPort() const { return _port; }
const std::string PostgressDatabase::getBaseName() const { return _baseName; }
const std::string PostgressDatabase::getUser() const { return _user; }
const std::string PostgressDatabase::getPassword() const { return _password; }

const std::string PostgressDatabase::getConnectionString() const { return _connectionString; }

// setters

void PostgressDatabase::makeConnection() {
  conn_ = PQconnectdb(_connectionString.c_str());

  if (PQstatus(conn_) != CONNECTION_OK) {
    std::cerr << "[DB ERROR] Connection failed: " << PQerrorMessage(conn_);
  } else {
    std::cout << "[DB INFO] Connection successfull\n";
  }
}

void PostgressDatabase::setHost(const std::string &host) { _host = host; }
void PostgressDatabase::setPort(const int &port) { _port = port; }
void PostgressDatabase::setBaseName(const std::string &baseName) { _baseName = baseName; }
void PostgressDatabase::setUser(const std::string &user) { _user = user; }
void PostgressDatabase::setPassword(const std::string &password) { _password = password; }

void PostgressDatabase::setConnectionString(const std::string &connectionString) {
  _connectionString = connectionString;
}

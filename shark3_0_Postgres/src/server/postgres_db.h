#pragma once

#include <libpq-fe.h>
#include <string>

class PostgressDatabase {
private:
  PGconn *conn_;
  std::string _host;
  int _port;
  std::string _baseName;
  std::string _user;
  std::string _password;

  std::string _connectionString;

public:
  explicit PostgressDatabase() = default;

  ~PostgressDatabase();

  // getters
  bool isConnected() const;
  PGconn *getConnection() const;

  const std::string getHost() const;
  const int getPort() const;
  const std::string getBaseName() const;
  const std::string getUser() const;
  const std::string getPassword() const;

  const std::string getConnectionString() const;

  // setters

  void makeConnection();

  void setHost(const std::string &host);
  void setPort(const int &port);
  void setBaseName(const std::string &baseName);
  void setUser(const std::string &user);
  void setPassword(const std::string &password);

  void setConnectionString(const std::string &connectionString);
};
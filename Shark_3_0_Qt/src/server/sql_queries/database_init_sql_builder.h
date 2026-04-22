#pragma once

#include <map>
#include <string>

class DatabaseInitSqlBuilder {
 public:
  DatabaseInitSqlBuilder() = default;

  std::multimap<int, std::string> BuildSchemaSql() const;
  std::multimap<int, std::string> BuildChatFirstSql() const;
  std::multimap<int, std::string> BuildChatSecondSql() const;
};

#pragma once
#include <filesystem>
#include <string>
#include <sqlite3.h>

// Determine the environment variable
std::string getEnvironmentName();

// Determine the user's home directory
std::string getHomeDir();

// Base storage directory
std::filesystem::path getDbDirPath(const std::string &homeDir, const std::string &envName);

// Create the directories and the database
bool ensureDbDirExists(const std::filesystem::path &dbDir, std::string &errorMsg);

// Build the full file path
std::filesystem::path getDbFilePath(const std::filesystem::path &dbDir);

// Create the SQLite database
bool openClientDb(const std::filesystem::path &dbFile,
                  sqlite3 **outDb,
                  std::string &errorMsg);
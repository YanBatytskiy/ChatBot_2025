#include "client_sql_lite.h"
#include <cstdlib> // std::getenv
#include <pwd.h>   // getpwuid, passwd
#include <string>
#include <unistd.h> // getuid

#if defined(__unix__) || defined(__APPLE__)
#include <sys/stat.h> // chmod
#include <unistd.h>
#endif

// Determine the environment variable
std::string getEnvironmentName() {
  const char *envVar = std::getenv("SHARK_ENV");
  return (envVar && *envVar) ? envVar : "prod";
}

// Determine the user's home directory
std::string getHomeDir() {
  if (const char *h = std::getenv("HOME"); h && *h) {
    return std::string(h);
  }
  if (passwd *pw = getpwuid(getuid()); pw && pw->pw_dir && *pw->pw_dir) {
    return std::string(pw->pw_dir);
  }
  return std::string(); // empty string = failure to determine HOME
}

// Base storage directory
std::filesystem::path getDbDirPath(const std::string &homeDir, const std::string &envName) {
  // Validate input arguments: an empty homeDir cannot be used.
  if (homeDir.empty())
    return {};

#ifdef __APPLE__
  // macOS: ~/Library/Application Support/shark3/<ENV>/
  std::filesystem::path base = std::filesystem::path(homeDir) / "Library" / "Application Support" / "shark3";
#else
  // Linux: ~/.local/share/shark3/<ENV>/
  std::filesystem::path base = std::filesystem::path(homeDir) / ".local" / "share" / "shark3";
#endif

  // Append the environment subdirectory.
  return base / envName;
}

// Create the directories and the database
bool ensureDbDirExists(const std::filesystem::path &dbDir,
                       std::string &errorMsg) {
  if (dbDir.empty()) {
    errorMsg = "dbDir is empty";
    return false;
  }

  std::error_code ec;
  // Create all missing path segments; if the directory already exists it is not an error.
  std::filesystem::create_directories(dbDir, ec);
  if (ec) {
    errorMsg = "create_directories failed: " + ec.message();
    return false;
  }

#if defined(__unix__) || defined(__APPLE__)
  // Recommended permissions 0700: owner only.
  if (::chmod(dbDir.c_str(), 0700) != 0) {
    // Not fatal: record a warning in errorMsg but treat as success.
    errorMsg = "chmod(0700) warning: errno=" + std::to_string(errno);
  }
#endif

  return true;
}

// Build the full file path
std::filesystem::path getDbFilePath(const std::filesystem::path &dbDir) {
  if (dbDir.empty())
    return {};                // empty input = caller error
  return dbDir / "client.db"; // append the file name to the directory
}

// Create the SQLite database
bool openClientDb(const std::filesystem::path &dbFile,
                  sqlite3 **outDb,
                  std::string &errorMsg) {
  if (dbFile.empty()) {
    errorMsg = "dbFile path is empty";
    return false;
  }
  if (!outDb) {
    errorMsg = "outDb pointer is null";
    return false;
  }

  int flags = SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
  sqlite3 *db = nullptr;
  int rc = sqlite3_open_v2(dbFile.string().c_str(), &db, flags, nullptr);

  if (rc != SQLITE_OK) {
    errorMsg = db ? sqlite3_errmsg(db) : "sqlite3_open_v2 failed";
    if (db)
      sqlite3_close(db);
    return false;
  }

  *outDb = db;
  return true;
}
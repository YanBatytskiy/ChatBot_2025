#pragma once
#include <filesystem>
#include <string>
#include <sqlite3.h>


//определяем переменную окружения
std::string getEnvironmentName();

// определить домашний каталог пользователя
std::string getHomeDir();

// базовый каталог хранения
std::filesystem::path getDbDirPath(const std::string &homeDir, const std::string &envName);

// создать каталоги и бд
bool ensureDbDirExists(const std::filesystem::path &dbDir, std::string &errorMsg);

// формируем полный путь к файлу
std::filesystem::path getDbFilePath(const std::filesystem::path &dbDir);

// создаем базу SQL Lite
bool openClientDb(const std::filesystem::path& dbFile,
                  sqlite3** outDb,
                  std::string& errorMsg);
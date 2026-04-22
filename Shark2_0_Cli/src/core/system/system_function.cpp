#include "system_function.h"
#include "exception/validation_exception.h"
#include "message/message_content_struct.h"
#include <algorithm>
#include <cctype>
#include <ctime>
#include <iostream>
#include <limits>
#include <locale>
#include <string>
#include <sys/utsname.h>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
// #include <sys/utsname.h>
#elif defined(_APPLE_)
// #include <sys/utsname.h>
#endif

const std::string getSystemType() {
#if defined(_WIN32)
  return "win";
#elif defined(__linux__)
  return "linux";
#elif defined(__APPLE__)
  return "osx";
#else
  return "unknown";
#endif
}

void printSystemName() {
  std::string systemType = getSystemType();

#if defined(__linux__) || defined(__APPLE__)
  if (systemType == "linux" || systemType == "osx") {
    struct utsname utsname;
    uname(&utsname);

    std::cout << "OS name: " << utsname.sysname << ", OS Release: " << utsname.release << std::endl
              << ", OS version: " << utsname.version << std::endl;
    std::cout << "Architecture: " << utsname.machine << ", Host name: " << utsname.nodename << std::endl;
  }
#endif

#if defined(_WIN32)
  if (systemType == "win") {
    wchar_t computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
    if (GetComputerNameW(computerName, &size)) {
      std::wcout << L"Host name: " << computerName << std::endl;
    } else {
      std::wcerr << L"Failed to get computer name" << std::endl;
    }

    SYSTEM_INFO sysInfo;
    GetNativeSystemInfo(&sysInfo);

    std::wcout << L"Architecture: ";
    switch (sysInfo.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64:
      std::wcout << L"x64 (AMD/Intel)";
      break;
    case PROCESSOR_ARCHITECTURE_INTEL:
      std::wcout << L"x86";
      break;
    case PROCESSOR_ARCHITECTURE_ARM64:
      std::wcout << L"ARM64";
      break;
    default:
      std::wcout << L"Unknown";
      break;
    }
    std::wcout << std::endl;
  }
#endif
}
/**
 * @brief Получает индекс символа в алфавите.
 * @param word Строка, содержащая символ (может быть многобайтный UTF-8).
 * @return Индекс символа в массиве alphabet или -1, если не найден.
 */
int getCharIndex(const std::string &word) {
  if (word.empty())
    return -1;
  std::size_t charLen = getUtf8CharLen((unsigned char)word[0]);

  if (charLen > word.size())
    return -1;

  std::string ch = word.substr(0, charLen);

  for (int i = 0; i < ALPHABET_SIZE; ++i)
    if (ch == alphabet[i])
      return i;

  return -1;
}

/**
 * @brief Определяет длину UTF-8 символа по первому байту.
 * @param ch Первый байт символа.
 * @return Количество байт в символе (1-4).
 */
std::size_t getUtf8CharLen(unsigned char ch) {
  if ((ch & 0x80) == 0)
    return 1;
  else if ((ch & 0xE0) == 0xC0)
    return 2;
  else if ((ch & 0xF0) == 0xE0)
    return 3;
  else if ((ch & 0xF8) == 0xF0)
    return 4;
  return 1;
}

/**
 * @brief Включает поддержку UTF-8 в консоли.
 *
 * @details Настраивает глобальную локаль и кодировку консоли.
 * Работает стабильно на Windows (MSYS2/MinGW), Linux и macOS.
 */
void enableUTF8Console() {
#if defined(_WIN32)
  try {
    std::locale utf8_locale(""); // системная локаль (обычно UTF-8 в MSYS2)
    std::locale::global(utf8_locale);

    std::wcin.imbue(utf8_locale);
    std::wcout.imbue(utf8_locale);
    std::wcerr.imbue(utf8_locale);
  } catch (const std::exception &e) {
    std::cerr << "Failed to set UTF-8 locale on Windows: " << e.what() << std::endl;
  }

  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);

#else
  try {
    std::locale utf8_locale("en_US.UTF-8");
    std::locale::global(utf8_locale);

    std::wcin.imbue(utf8_locale);
    std::wcout.imbue(utf8_locale);
    std::wcerr.imbue(utf8_locale);
  } catch (const std::exception &e) {
    std::cerr << "Failed to set UTF-8 locale on Unix: " << e.what() << std::endl;
  }
#endif
}

/**
 * @brief Converts a string to an integer.
 * @param str The string to convert.
 * @return The converted integer value.
 * @throws NonDigitalCharacterException If the string contains non-numeric
 * characters.
 * @throws IndexOutOfRangeException If the value exceeds the integer range.
 */
int parseGetlineToInt(const std::string &str) {
  try {
    long long value = std::stoll(str);

    if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max()) {
      throw exc::IndexOutOfRangeException(str);
    }

    return static_cast<int>(value);

  } catch (const std::invalid_argument &) {
    throw exc::IndexOutOfRangeException("Недопустимые символы в строке: \"" + str + "\"");
  }
}
std::size_t parseGetlineToSizeT(const std::string &str) { // конвертация из string в size_t
  try {
    unsigned long long value = std::stoull(str);

    if (value > std::numeric_limits<std::size_t>::max()) {
      throw exc::IndexOutOfRangeException(str);
    }

    return static_cast<std::size_t>(value);

  } catch (const std::invalid_argument &) {
    throw exc::NonDigitalCharacterException();
  } catch (const std::out_of_range &) {
    throw exc::IndexOutOfRangeException(str);
  }
}

/**
 * @brief Converts a string to lowercase.
 * @param str The input string.
 * @return The lowercase version of the input string.
 */
std::string TextToLower(const std::string &str) {
  std::string result = str;

  // приводим всех к нижнему регистру
  std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
  return result;
}

Message createOneMessage(const std::string &textContent, const std::shared_ptr<User> &sender, const int64_t &timeStamp,
                         std::size_t messageId) {

  std::vector<std::shared_ptr<IMessageContent>> iMessageContent;

  MessageContent<TextContent> messageContentText(textContent);

  iMessageContent.push_back(std::make_shared<MessageContent<TextContent>>(messageContentText));

  if (!sender)
    throw exc::UnknownException(" Отправитель отсутствует. Сообщение не будет "
                                "создано. addMessageToChat");

  Message message(iMessageContent, sender, timeStamp, messageId);
  return message;
}

// проверяем только на англ буквы и цифры
bool engAndFiguresCheck(const std::string &inputData) {
  for (std::size_t i = 0; i < inputData.size();) {
    std::size_t charLen = getUtf8CharLen(static_cast<unsigned char>(inputData[i]));

    if (charLen != 1)
      return false; // не ASCII

    char ch = inputData[i];
    // не цифра и не буква
    if (!std::isalnum(static_cast<unsigned char>(ch)))
      return false;

    ++i;
  }
  return true;
}

// проверяем только на цифры
bool figuresCheck(const std::string &inputData) {
  for (std::size_t i = 0; i < inputData.size();) {
    std::size_t charLen = getUtf8CharLen(static_cast<unsigned char>(inputData[i]));

    if (charLen != 1)
      return false; // не ASCII

    char ch = inputData[i];
    // не цифра и не буква
    if (!std::isdigit(static_cast<unsigned char>(ch)))
      return false;

    ++i;
  }
  return true;
}
// проверяем только на цифры
bool figuresAndCommaCheck(const std::string &inputData) {
  for (std::size_t i = 0; i < inputData.size();) {
    std::size_t charLen = getUtf8CharLen(static_cast<unsigned char>(inputData[i]));

    if (charLen != 1)
      return false; // не ASCII

    char ch = inputData[i];
    // не цифра и не буква
    if (!std::isdigit(static_cast<unsigned char>(ch)) && ch != ',')
      return false;

    ++i;
  }
  return true;
}
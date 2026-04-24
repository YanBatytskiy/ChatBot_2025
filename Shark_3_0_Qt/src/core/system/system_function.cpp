#include "system_function.h"
#include "exceptions_cpp/validation_exception.h"
#include "message/message_content_struct.h"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <limits>
#include <locale>
#include <string>
#include <sys/utsname.h>

const std::string getSystemType() {
#if defined(__linux__)
  return "linux";
#elif defined(__APPLE__)
  return "osx";
#else
  return "unknown";
#endif
}

void printSystemName() {
  std::string systemType = getSystemType();

  if (systemType == "linux" || systemType == "osx") {
    struct utsname utsname;
    uname(&utsname);

    std::cout << "OS name: " << utsname.sysname << ", OS Release: " << utsname.release << std::endl
              << ", OS version: " << utsname.version << std::endl;
    std::cout << "Architecture: " << utsname.machine << ", Host name: " << utsname.nodename << std::endl;
  }
}
/**
 * @brief Returns the index of a character in the alphabet.
 * @param word String containing a character (may be multi-byte UTF-8).
 * @return Index of the character in the alphabet array, or -1 if not found.
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
 * @brief Determines the length of a UTF-8 character from the first byte.
 * @param ch The first byte of the character.
 * @return The number of bytes in the character (1-4).
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
 * @brief Enables UTF-8 support in the console.
 *
 * @details Configures the global locale and the console encoding.
 * Works reliably on Windows (MSYS2/MinGW), Linux and macOS.
 */
void enableUTF8Console() {
  try {
    std::locale utf8_locale("en_US.UTF-8");
    std::locale::global(utf8_locale);

    std::wcin.imbue(utf8_locale);
    std::wcout.imbue(utf8_locale);
    std::wcerr.imbue(utf8_locale);
  } catch (const std::exception &e) {
    std::cerr << "Failed to set UTF-8 locale on Unix: " << e.what() << std::endl;
  }
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
    throw exc::IndexOutOfRangeException("Invalid characters in string: \"" + str + "\"");
  }
}
std::size_t parseGetlineToSizeT(const std::string &str) { // convert from string to size_t
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

  // convert all to lower case
  std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
  return result;
}

Message createOneMessage(const std::string &textContent, const std::shared_ptr<User> &sender, const int64_t &timeStamp,
                         std::size_t messageId) {

  std::vector<std::shared_ptr<IMessageContent>> iMessageContent;

  MessageContent<TextContent> messageContentText(textContent);

  iMessageContent.push_back(std::make_shared<MessageContent<TextContent>>(messageContentText));

  if (!sender)
    throw exc::UnknownException(" Sender is absent. The message will not be "
                                "created. addMessageToChat");

  Message message(iMessageContent, sender, timeStamp, messageId);
  return message;
}

// validate only against English letters and digits
bool engAndFiguresCheck(const std::string &inputData) {

  for (std::size_t i = 0; i < inputData.size();) {
    std::size_t charLen = getUtf8CharLen(static_cast<unsigned char>(inputData[i]));

    if (charLen != 1)
      return false; // not ASCII

    char ch = inputData[i];
    // not a digit and not a letter
    if (!std::isalnum(static_cast<unsigned char>(ch)))
      return false;

    ++i;
  }
  return true;
}

bool checkNewLoginPasswordForLimits(const std::string &inputData, std::size_t contentLengthMin,
                                    std::size_t contentLengthMax, bool isPassword) {

  bool isCapital = false, isNumber = false;
  std::size_t utf8SymbolCount = 0;

  for (std::size_t i = 0; i < inputData.size();) {

    std::size_t charLen = getUtf8CharLen(static_cast<unsigned char>(inputData[i]));

    if (i + charLen > inputData.size())
      throw exc::InvalidCharacterException("");

    std::string utf8Char = inputData.substr(i, charLen);

    ++utf8SymbolCount;

    if (charLen == 1) {
      char ch = utf8Char[0];
      if (std::isdigit(ch))
        isNumber = true;

      if (std::isupper(ch))
        isCapital = true;
    } // if
    else
      return false;

    i += charLen;

  } // for i

  if (utf8SymbolCount < contentLengthMin || utf8SymbolCount > contentLengthMax)
    return false;

  if (isPassword) {
    if (!isCapital)
      return false;
    if (!isNumber)
      return false;
  }

  return true;
}

// validate only against digits
bool figuresCheck(const std::string &inputData) {
  for (std::size_t i = 0; i < inputData.size();) {
    std::size_t charLen = getUtf8CharLen(static_cast<unsigned char>(inputData[i]));

    if (charLen != 1)
      return false; // not ASCII

    char ch = inputData[i];
    // not a digit and not a letter
    if (!std::isdigit(static_cast<unsigned char>(ch)))
      return false;

    ++i;
  }
  return true;
}
// validate only against digits and comma
bool figuresAndCommaCheck(const std::string &inputData) {
  for (std::size_t i = 0; i < inputData.size();) {
    std::size_t charLen = getUtf8CharLen(static_cast<unsigned char>(inputData[i]));

    if (charLen != 1)
      return false; // not ASCII

    char ch = inputData[i];
    // not a digit and not a letter
    if (!std::isdigit(static_cast<unsigned char>(ch)) && ch != ',')
      return false;

    ++i;
  }
  return true;
}

// escape the value for a query
std::string makeStringForSQL(const std::string &inputData) {

  std::string result = inputData;

  for (std::size_t pos = 0; (pos = result.find('\'', pos)) != std::string::npos; pos += 2) {
    result.replace(pos, 1, "''");
  }
  return result;
}

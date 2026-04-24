#pragma once
#include "message/message.h"
#include <cstddef>
#include <cstdint>
#include <string>

#define ALPHABET_SIZE 59 ///< Alphabet size (Latin + Cyrillic)
#define ENG_SIZE 26      ///< Number of characters in the English alphabet
#define RUS_SIZE 32      ///< Number of characters in the Russian alphabet

/**
 * @brief Global alphabet for UTF-8 characters (Latin a-z and Cyrillic a-ya).
 */
const std::string alphabet[ALPHABET_SIZE] = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o",
                                             "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "\u0430", "\u0431", "\u0432", "\u0433",
                                             "\u0434", "\u0435", "\u0451", "\u0436", "\u0437", "\u0438", "\u0439", "\u043A", "\u043B", "\u043C", "\u043D", "\u043E", "\u043F", "\u0440", "\u0441",
                                             "\u0442", "\u0443", "\u0444", "\u0445", "\u0446", "\u0447", "\u0448", "\u0449", "\u044A", "\u044B", "\u044C", "\u044D", "\u044E", "\u044F"};

const std::string getSystemType();

void printSystemName();

/**
 * @brief Returns the length of a UTF-8 character.
 * @param ch The first byte of the character
 * @return The number of bytes in the character
 */
std::size_t getUtf8CharLen(unsigned char ch);

/**
 * @brief Returns the index of a character in the alphabet.
 * @param ch UTF-8 character
 * @return Index, or -1 if the character is not found
 */
int getCharIndex(const std::string &ch);

/**
 * @brief Enables UTF-8 support in the console.
 *
 * @details Configures the global locale and the console encoding.
 * Works reliably on Windows (MSYS2/MinGW), Linux and macOS.
 */
void enableUTF8Console();

/**
 * @brief Enum class defining message target types.
 */
enum class MessageTarget { One,
                           Several };

/**
 * @brief Converts a string to an integer.
 * @param str The string to convert.
 * @return The converted integer value.
 */
int parseGetlineToInt(const std::string &str); // convert from string to int

size_t parseGetlineToSizeT(const std::string &str); // convert from string to size_t

/**
 * @brief Converts a string to lowercase.
 * @param str The input string.
 * @return The lowercase version of the input string.
 */
std::string TextToLower(const std::string &str); // convert to lower case

Message createOneMessage(const std::string &textContent, const std::shared_ptr<User> &sender, const int64_t &timeStamp,
                         std::size_t messageId);
//      //validate only against English letters and digits

bool engAndFiguresCheck(const std::string &inputData);

bool checkNewLoginPasswordForLimits(const std::string &inputData, std::size_t contentLengthMin,
                                    std::size_t contentLengthMax, bool isPassword);

// validate only against digits
bool figuresCheck(const std::string &inputData);

// validate only against digits and comma
bool figuresAndCommaCheck(const std::string &inputData);

// escape the value for a query
std::string makeStringForSQL(const std::string &inputData);

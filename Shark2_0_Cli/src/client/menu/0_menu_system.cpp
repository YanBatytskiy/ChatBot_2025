#include "chat/chat.h"
#include "client/client_session.h"
#include "exception/validation_exception.h"
#include "message/message_content_struct.h"
#include "system/date_time_utils.h"
#include "system/system_function.h"
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>

std::optional<Message> inputNewMessage(ClientSession &clientSession, std::shared_ptr<Chat> chat) {

  std::cout << std::endl << "Наберите новое сообщение либо 0 для выхода:" << std::endl;
  std::string inputData;

  while (true) {
    try {
      std::getline(std::cin, inputData);

      if (inputData.empty())
        throw exc::EmptyInputException();

      if (inputData == "0")
        return std::nullopt;

      std::vector<std::shared_ptr<User>> recipients;

      for (const auto &participant : chat->getParticipants()) {
        auto user_ptr = participant._user.lock();

        if (user_ptr) {
          if (user_ptr != clientSession.getActiveUserCl())
            recipients.push_back(user_ptr);
        }
      }

      std::vector<std::shared_ptr<IMessageContent>> iMessageContent;
      TextContent textContent(inputData);
      MessageContent<TextContent> messageContentText(textContent);
      iMessageContent.push_back(std::make_shared<MessageContent<TextContent>>(messageContentText));

      Message message(iMessageContent, clientSession.getActiveUserCl(), getCurrentDateTimeInt(), 0);

      return message;

    } catch (const exc::ValidationException &ex) {
      std::cout << " ! " << ex.what() << " Попробуйте еще раз." << std::endl;
      continue;
    }
  } // while
}

bool checkNewDataInputForLimits(const std::string &inputData, std::size_t contentLengthMin,
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
      throw exc::InvalidCharacterException("");

    i += charLen;

  } // for i

  if (utf8SymbolCount < contentLengthMin || utf8SymbolCount > contentLengthMax)
    throw exc::InvalidQuantityCharacterException();

  if (isPassword) {
    if (!isCapital)
      throw exc::NonCapitalCharacterException();
    if (!isNumber)
      throw exc::NonDigitalCharacterException();
  }

  return true;
}

std::string inputDataValidation(const std::string &prompt, std::size_t dataLengthMin, std::size_t dataLengthMax,
                                bool isPassword, bool newUserData) {
  std::string inputData;

  // доделать проверку на utf-8 символы английского языка
  while (true) {
    std::cout << prompt << std::endl;
    std::getline(std::cin, inputData);
    try {
      if (inputData.empty())
        throw exc::EmptyInputException();

      if (inputData == "0")
        return inputData;

      // проверяем только на англ буквы и цифры
      if (!engAndFiguresCheck(inputData))
        throw exc::InvalidCharacterException("");

      if (newUserData)
        checkNewDataInputForLimits(inputData, dataLengthMin, dataLengthMax, isPassword);

      return inputData;
    } catch (const exc::ValidationException &ex) {
      std::cout << " ! " << ex.what() << " Попробуйте еще раз." << std::endl;
      continue;
    }
  }
}

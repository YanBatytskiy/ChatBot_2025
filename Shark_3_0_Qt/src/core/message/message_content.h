#pragma once

/**
 * @brief Interface for message content types.
 */
class IMessageContent { // interface for template MessageContent
public:
  /**
   * @brief Default virtual destructor.
   */
  virtual ~IMessageContent() = default;
};

/**
 * @brief Template class for message content of a specific type.
 * @tparam T The type of content (e.g., TextContent, FileContent, ImageContent).
 */
template <typename T> class MessageContent : public IMessageContent {
private:
  T _content; // will take type according to the used struct of message content

public:
  /**
   * @brief Constructor for MessageContent.
   * @param content The content of type T to initialize the message.
   */
  explicit MessageContent(const T &content) : _content(content) {}; // constructor

  /**
   * @brief Gets the message content.
   * @return Reference to the content of type T.
   */
  T &getMessageContent() { return _content; };

  /**
   * @brief Gets the message content (const version).
   * @return Const reference to the content of type T.
   */
  const T &getMessageContent() const { return _content; };

  /**
   * @brief Sets the message content (to be implemented).
   * @param content The new content of type T.
   * @details Placeholder for future message editing functionality.
   */
  void setMessageContent(const T &content) {}; // for edit of message, will be realized further
};


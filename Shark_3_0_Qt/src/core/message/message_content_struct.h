#pragma once
#include <string>

/**
 * @brief Structure for text content in a message.
 */
struct TextContent {
  std::string _text; ///< Text content.

  /**
   * @brief Constructor for TextContent.
   * @param text The text content.
   */
  TextContent(const std::string &text) : _text(text) {};

  /**
   * @brief Default destructor.
   */
  ~TextContent() = default;
};

/**
 * @brief Structure for file content in a message (to be implemented).
 */
struct FileContent { // will be realized further
  std::string _fileName; ///< Name of the file.

  /**
   * @brief Constructor for FileContent.
   * @param fileName Name of the file.
   */
  FileContent(const std::string &fileName) : _fileName(fileName) {};

  /**
   * @brief Default destructor.
   */
  ~FileContent() = default;
};

/**
 * @brief Structure for image content in a message (to be implemented).
 */
struct ImageContent { // will be realized further
  std::string _image; ///< Image identifier or path.

  /**
   * @brief Constructor for ImageContent.
   * @param image Image identifier or path.
   */
  ImageContent(const std::string &image) : _image(image) {};

  /**
   * @brief Default destructor.
   */
  ~ImageContent() = default;
};


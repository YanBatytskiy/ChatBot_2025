#ifndef MODEL_CHAT_MESSAGES_H
#define MODEL_CHAT_MESSAGES_H

#include <QAbstractListModel>

class MessageModel final : public QAbstractListModel {
  Q_OBJECT
private:

  struct MessageItem {
    QString MessageText;
    QString SenderLogin;
    QString SenderName;
    std::int64_t TimeStamp;
    std::size_t MessageId;
  };
  std::vector<MessageItem> _items;


public:

  enum MessageRoles: int {
    MessageTextRole = Qt::UserRole +1,
    SenderLoginRole,
    SenderNameRole,
    TimeStampRole,
    MessageIdRole,
  };

  Q_ENUM(MessageRoles)

  explicit MessageModel(QObject *parent = nullptr);

  QHash<int, QByteArray> roleNames() const override;
  int rowCount(const QModelIndex& parent = {}) const override;
  QVariant data(const QModelIndex& index, int messageRole) const override;
  Qt::ItemFlags flags(const QModelIndex& idx) const override;

  void appendItem(const MessageItem& messageItem);
  void setMessageText(int row, const QString& value);
  void setSenderLogin(int row, const QString& value);
  void setSenderName(int row, const QString& value);
  void setTimeStamp(int row, std::int64_t value);
  void setMessageId(int row, std::size_t value);
  void clear();

  void fillMessageItem(const QString &messageText, const QString &senderLogin,
                       const QString &senderName,
                       const std::int64_t timeStamp,
                       std::size_t messageId);
};

#endif // MODEL_CHAT_MESSAGES_H

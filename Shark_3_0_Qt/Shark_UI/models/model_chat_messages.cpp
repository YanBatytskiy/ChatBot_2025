#include "model_chat_messages.h"

MessageModel::MessageModel(QObject *parent) : QAbstractListModel(parent) {}

QHash<int, QByteArray> MessageModel::roleNames() const {
  return {{MessageTextRole, "messageText"},
          {SenderLoginRole, "senderLogin"},
          {SenderNameRole, "senderName"},
          {TimeStampRole, "timeStamp"},
          {MessageIdRole, "messageId"}};
}

int MessageModel::rowCount(const QModelIndex &parent) const
{
  if (parent.isValid()) return 0;
  return static_cast<int>(_items.size());
}

QVariant MessageModel::data(const QModelIndex &index, int messageRole) const
{
  if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) return {};
  const MessageItem& it = _items[static_cast<size_t>(index.row())];

  switch (messageRole) {
  case MessageTextRole :  return it.MessageText;
  case SenderLoginRole :  return it.SenderLogin;
  case SenderNameRole :  return it.SenderName;
  case TimeStampRole :  return static_cast<qlonglong>(it.TimeStamp);
  case MessageIdRole :  return static_cast<qlonglong>(it.MessageId);
  }
  return {};
}

Qt::ItemFlags MessageModel::flags(const QModelIndex &idx) const
{
  if (!idx.isValid()) return Qt::NoItemFlags;
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void MessageModel::appendItem(const MessageItem &messageItem)
{
  const int rows = rowCount();
  beginInsertRows({},rows,rows);
  _items.push_back(messageItem);
  endInsertRows();
}

void MessageModel::setMessageText(int row, const QString &value)
{
  if (row<0 || row >=rowCount()) return;
  _items[static_cast<size_t>(row)].MessageText = value;
  const QModelIndex i = index(row);
  emit dataChanged(i, i, {MessageTextRole});
}

void MessageModel::setSenderLogin(int row, const QString &value)
{
  if (row<0 || row >=rowCount()) return;
  _items[static_cast<size_t>(row)].SenderLogin = value;
  const QModelIndex i = index(row);
  emit dataChanged(i, i, {SenderLoginRole});

}

void MessageModel::setSenderName(int row, const QString &value)
{
  if (row<0 || row >=rowCount()) return;
  _items[static_cast<size_t>(row)].SenderName= value;
  const QModelIndex i = index(row);
  emit dataChanged(i, i, {SenderNameRole});

}

void MessageModel::setTimeStamp(int row, std::int64_t value)
{
  if (row <0 || row >=rowCount()) return;
  _items[static_cast<size_t>(row )].TimeStamp = value;
  const QModelIndex i = index(row);
  emit dataChanged(i, i, {TimeStampRole});
}

void MessageModel::setMessageId(int row, std::size_t value)
{
  if (row <0 || row >=rowCount()) return;
  _items[static_cast<size_t>(row )].MessageId = value;
  const QModelIndex i = index(row);
  emit dataChanged(i, i, {MessageIdRole});
}

void MessageModel::clear()
{
  const int n = rowCount();
  if (n == 0) return;
  beginRemoveRows(QModelIndex(), 0, n - 1);
  _items.clear();
  endRemoveRows();}

void MessageModel::fillMessageItem(const QString &messageText,
                                   const QString &senderLogin,
                                   const QString &senderName,
                                   const std::int64_t timeStamp, std::size_t messageId)
{
  MessageItem messageItem;
  messageItem.MessageText = messageText;
  messageItem.SenderLogin = senderLogin;
  messageItem.SenderName = senderName;
  messageItem.TimeStamp = timeStamp;
  messageItem.MessageId = messageId;

  appendItem(messageItem);
};

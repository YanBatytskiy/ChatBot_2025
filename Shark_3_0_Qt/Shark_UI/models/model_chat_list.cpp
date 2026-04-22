#include "model_chat_list.h"
#include "system/date_time_utils.h"

ChatListModel::ChatListModel(QObject *parent) :QAbstractListModel(parent){}

QHash<int, QByteArray> ChatListModel::roleNames() const {
  return {{ParticipantsChatListRole, "participantsList"},
          {InfoTextRole, "infoText"},
          {UnreadCountRole, "unreadCount"},
          {IsMutedRole, "isMuted"},
          {LastTimeRole, "lastTime"},
          {ChatIdRole, "chatId"}
  };
}

int ChatListModel::rowCount(const QModelIndex &parent) const
{
  if (parent.isValid()) return 0;
  return static_cast<int>(_items.size());
}

void ChatListModel::appendItem(const ChatListItem &chatItem)
{
  const int rows = rowCount();
  beginInsertRows({},rows,rows);
  _items.push_back(chatItem);
  endInsertRows();
}

QVariant ChatListModel::data(const QModelIndex &index, int chatRole) const
{
  if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) return {};
  const ChatListItem& it = _items[static_cast<size_t>(index.row())];
  switch (chatRole) {
  case ParticipantsChatListRole: return it.ParticipantsChatList;
  case InfoTextRole:             return it.InfoText;
  case UnreadCountRole:          return it.UnreadCount;
  case IsMutedRole:              return it.isMuted;
  case LastTimeRole:             return static_cast<qlonglong>(it.LastTime);
  case ChatIdRole:             return static_cast<qlonglong>(it.ChatId);
  }
  return {};
}

void ChatListModel::setParticipantsChatList(int row, const QString &textValue)
{
  if (row<0 || row >=rowCount()) return;
  _items[static_cast<size_t>(row)].ParticipantsChatList = textValue;
  const QModelIndex i = index(row);
  emit dataChanged(i, i, {ParticipantsChatListRole});
}

void ChatListModel::setInfoText(int row, const QString &textValue)
{
  if (row <0 || row >=rowCount()) return;
  _items[static_cast<size_t>(row)].InfoText = textValue;
  const QModelIndex i = index(row);
  emit dataChanged(i, i, {InfoTextRole});
}

void ChatListModel::setUnreadCount(int row, int newValue)
{
  if (row <0 || row >=rowCount()) return;

  _items[static_cast<size_t>(row)].UnreadCount = newValue;

  const QModelIndex i = index(row);

  _items[row].InfoText = buildInfoTextForRow(QString::number(_items[row].ChatId),
                                             QString::number(newValue),
                                             QString::fromStdString(
                                                 formatTimeStampToString(_items[row].LastTime, true)));


emit dataChanged(i, i, {InfoTextRole, UnreadCountRole});}

void ChatListModel::setIsMuted(int row, bool newValue)
{
  if (row <0 || row >=rowCount()) return;
  _items[static_cast<size_t>(row)].isMuted = newValue;
  const QModelIndex i = index(row);
  emit dataChanged(i, i, {IsMutedRole});
}

void ChatListModel::setLastTime(int row, std::int64_t timeValue)
{
  if (row <0 || row >=rowCount()) return;
  _items[static_cast<size_t>(row )].LastTime = timeValue;
  const QModelIndex i = index(row);

  _items[row].InfoText = buildInfoTextForRow(QString::number(_items[row].ChatId),
                                             QString::number(_items[row].UnreadCount),
                                             QString::fromStdString(
                                                 formatTimeStampToString(timeValue, true)));

  emit dataChanged(i, i, {InfoTextRole, LastTimeRole});
}

void ChatListModel::setChatId(int row, std::size_t Value)
{
  if (row <0 || row >=rowCount()) return;
  _items[static_cast<size_t>(row )].ChatId = Value;
  const QModelIndex i = index(row);
  emit dataChanged(i, i, {ChatIdRole});

}

void ChatListModel::clear()
{
  // beginResetModel();
  // _items.clear();
  // endResetModel();

  if (_items.empty()) return;
  const int last = rowCount() - 1;
  beginRemoveRows(QModelIndex(), 0, last);
  _items.clear();
  endRemoveRows();

}

QString ChatListModel::buildInfoTextForRow(const QString& chatIdStr, const QString& unreadCountStr, const QString& lastTimeStr )
{
  QString infoText;
  infoText = "Chat_Id = " + chatIdStr;
  infoText += ", новых = " + unreadCountStr;
  infoText += ", последнее: " + lastTimeStr;

  return infoText;
}

void ChatListModel::fillChatListItem(const QString& participantsChatList,
                                     const QString& infoText, const int& unreadCount, bool isMuted,
                                     std::int64_t lastTime, std::size_t chatId)
{
  ChatListItem chatListItem;
  chatListItem.ParticipantsChatList = participantsChatList;
  chatListItem.InfoText = infoText;
  chatListItem.UnreadCount = unreadCount;
  chatListItem.isMuted = isMuted;
  chatListItem.LastTime = lastTime;
  chatListItem.ChatId = chatId;

  appendItem(chatListItem);

}

Qt::ItemFlags ChatListModel::flags(const QModelIndex& idx) const {
  if (!idx.isValid()) return Qt::NoItemFlags;
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

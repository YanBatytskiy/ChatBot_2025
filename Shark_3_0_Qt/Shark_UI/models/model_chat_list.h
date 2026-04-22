#ifndef MODEL_CHAT_LIST_H
#define MODEL_CHAT_LIST_H

#include <QObject>
#include <QDateTime>
#include <QAbstractListModel>
#include <vector>
#include "dto_struct.h"

class ChatListModel final : public QAbstractListModel{
  Q_OBJECT

private:
  struct ChatListItem {
    QString ParticipantsChatList;
    QString InfoText;
    int UnreadCount;
    bool isMuted = false;
    std::int64_t LastTime;
    std::size_t ChatId;
  };
  std::vector<ChatListItem> _items;

public:
  enum ChatListRoles : int {
    ParticipantsChatListRole = Qt::UserRole + 1,
    InfoTextRole,
    UnreadCountRole,
    IsMutedRole,
    LastTimeRole,
    ChatIdRole
  };

  Q_ENUM(ChatListRoles)

  explicit ChatListModel(QObject *parent = nullptr);

  QHash<int, QByteArray> roleNames() const override;
  int rowCount(const QModelIndex& parent = {}) const override;
  QVariant data(const QModelIndex& index, int chatRole) const override;
  Qt::ItemFlags flags(const QModelIndex& idx) const override;

  void appendItem(const ChatListItem& chatItem);
  void setParticipantsChatList(int row, const QString& textValue);
  void setInfoText(int row, const QString& textValue);
  void setUnreadCount(int row, int newValue);
  void setIsMuted(int row, bool newValue);
  void setLastTime(int row, std::int64_t timeValue);
  void setChatId(int row, std::size_t Value);
  void clear();


  static QString buildInfoTextForRow(const QString& chatIdStr, const QString& unreadCountStr, const QString& lastTimeStr);

  void fillChatListItem(const QString& participantsChatList, const QString& infoText, const int& unreadCount, bool isMuted, std::int64_t lastTime, std::size_t chatId);

signals:


};

#endif // MODEL_CHAT_LIST_H

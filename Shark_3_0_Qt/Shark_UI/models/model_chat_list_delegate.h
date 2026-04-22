#ifndef MODEL_CHAT_LIST_DELEGATE_H
#define MODEL_CHAT_LIST_DELEGATE_H

#include <QObject>
#include <QStyledItemDelegate>

class ChatListItemDelegate final : public QStyledItemDelegate {
  Q_OBJECT
public:

  using QStyledItemDelegate::QStyledItemDelegate;

  QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

  void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

  ChatListItemDelegate();
};

#endif // MODEL_CHAT_LIST_DELEGATE_H

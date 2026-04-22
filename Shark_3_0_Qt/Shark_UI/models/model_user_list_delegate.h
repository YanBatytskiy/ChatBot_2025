#ifndef MODEL_USER_LIST_DELEGATE_H
#define MODEL_USER_LIST_DELEGATE_H

#include <QObject>
#include <QStyledItemDelegate>

class UserListItemDelegate final : public QStyledItemDelegate {
public:

  using QStyledItemDelegate::QStyledItemDelegate;

  QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

  void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

  UserListItemDelegate();

};

#endif // MODEL_USER_LIST_DELEGATE_H

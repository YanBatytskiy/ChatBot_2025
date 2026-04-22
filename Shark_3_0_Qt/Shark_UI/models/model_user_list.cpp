#include "model_user_list.h"

UserListModel::UserListModel(QObject *parent) : QAbstractListModel{parent} {}

QHash<int, QByteArray> UserListModel::roleNames() const {

  return {{LoginRole, "Login"},
          {NameRole, "Name"},         {EmailRole, "Email"},
          {PhoneRole, "Phone"},       {DisableReasonRole, "DisableReason"},
          {IsActiveRole, "IsActive"}, {DisableAtRole, "DisableAt"},
          {BunUntilRole, "BunUntil"}};
  }

int UserListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(_items.size());
}

QVariant UserListModel::data(const QModelIndex &index, int userRole) const
{
  if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) return {};
  const UserItem& it = _items[static_cast<size_t>(index.row())];
  switch (userRole) {
  case LoginRole:    return it.Login;
  case NameRole:    return it.Name;
  case EmailRole:    return it.Email;
  case PhoneRole:    return it.Phone;
  case DisableReasonRole:
    return it.DisableReason;
  case IsActiveRole:    return it.IsActive;
  case DisableAtRole:    return static_cast<qlonglong>(it.DisableAt);
  case BunUntilRole:
    return static_cast<qlonglong>(it.BunUntil);
  }
  return {};
}

Qt::ItemFlags UserListModel::flags(const QModelIndex &idx) const
{
  if (!idx.isValid()) return Qt::NoItemFlags;
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

UserListModel::UserItem UserListModel::getItem(int row) const
{
  if (row < 0 || row >= static_cast<int>(_items.size()))
    return{};
  return _items.at(row);
}

void UserListModel::appendItem(const UserItem &userItem)
{
  const int rows = rowCount();
  beginInsertRows({},rows,rows);
  _items.push_back(userItem);
  endInsertRows();
}

void UserListModel::removeItem(int row)
{
  if (row < 0 || row >= rowCount())
    return;

  beginRemoveRows(QModelIndex(), row, row);
  _items.erase(_items.begin() + row);
  endRemoveRows();}

void UserListModel::setDisableReasonUserList(int row, const QString &textValue)
{
  if (row<0 || row >=rowCount()) return;
  _items[static_cast<size_t>(row)].DisableReason = textValue;
  const QModelIndex i = index(row);
  emit dataChanged(i, i, {DisableReasonRole});
}

void UserListModel::setIsActiveUserList(int row, bool newValue) {
  if (row<0 || row >=rowCount()) return;
  _items[static_cast<size_t>(row)].IsActive = newValue;
  const QModelIndex i = index(row);
  emit dataChanged(i, i, {IsActiveRole});
}

void UserListModel::setBunUntilUserList(int row, std::int64_t newValue) {
  if (row<0 || row >=rowCount()) return;
  _items[static_cast<size_t>(row)].BunUntil = newValue;
  const QModelIndex i = index(row);
  emit dataChanged(i, i, {BunUntilRole});
}

void UserListModel::clear()
{
  if (rowCount() == 0) return;
  beginResetModel();
  _items.clear();
  endResetModel();
}

QString UserListModel::findNameByLogin(const QString &login) const
{
  for (int row = 0; row < rowCount(); ++row) {
    const QModelIndex idx = index(row, 0);
    if (idx.data(UserListModel::LoginRole).toString() == login) {
      return idx.data(UserListModel::NameRole).toString();
    }
  }
  return {};
}

void UserListModel::fillUserItem(const QString &login, const QString &name, const QString &email,
                                     const QString &phone, const QString &disableReason, bool isActive,
                                     const int64_t disableAt, const int64_t bunUntil) {
  UserItem userItem;
  userItem.Login = login;
  userItem.Name = name;
  userItem.Email = email;
  userItem.Phone = phone;
  userItem.DisableReason = disableReason;
  userItem.IsActive = isActive;
  userItem.DisableAt = disableAt;
  userItem.BunUntil = bunUntil;

  appendItem(userItem);
}

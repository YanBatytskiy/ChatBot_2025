#ifndef MODEL_USER_LIST_H
#define MODEL_USER_LIST_H

#include <QAbstractListModel>
#include <QObject>

class UserListModel final : public QAbstractListModel {
  Q_OBJECT

private:
  struct UserItem {
    QString Login;
    QString Name;
    QString Email;
    QString Phone;
    QString DisableReason;
    bool IsActive;
    std::int64_t DisableAt;
    std::int64_t BunUntil;
  };
  std::vector<UserItem> _items;

public:
  enum UserListRoles : int {
    LoginRole = Qt::UserRole + 1,
    NameRole,
    EmailRole,
    PhoneRole,
    DisableReasonRole,
    IsActiveRole,
    DisableAtRole,
    BunUntilRole
  };

  Q_ENUM(UserListRoles)

  explicit UserListModel(QObject *parent = nullptr);

  QHash<int, QByteArray> roleNames() const override;
  int rowCount(const QModelIndex& parent = {}) const override;
  QVariant data(const QModelIndex& index, int userRole) const override;
  Qt::ItemFlags flags(const QModelIndex& idx) const override;

  UserListModel::UserItem getItem(int row) const;

  void appendItem(const UserItem& userItem);
  void removeItem(int row);
  void setDisableReasonUserList(int row, const QString& textValue);
  void setIsActiveUserList(int row, bool newValue);
  void setBunUntilUserList(int row, std::int64_t newValue);
  void clear();
  QString findNameByLogin(const QString &login) const;

  void fillUserItem(const QString &login, const QString &name, const QString &email,
                        const QString &phone, const QString &disableReason, bool isActive, const int64_t disableAt,
                        const int64_t bunUntil);
};

#endif // MODEL_USER_LIST_H

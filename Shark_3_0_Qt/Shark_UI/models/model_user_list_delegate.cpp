#include "model_user_list_delegate.h"
#include <QApplication>
#include <QPainter>
#include <QTextLayout>
#include <QFontMetrics>
#include "system/date_time_utils.h"


namespace {
constexpr int kPaddingX = 12;
constexpr int kPaddingY = 6;
constexpr int kLineSpacing = 2;

// Палитра
const QColor kBg      ("#FFFACD"); // обычный фон
const QColor kHoverBg ("#F5F7FA"); // фон под мышью
const QColor kSelBg   ("#40A7E3"); // фон выделения
const QColor kSep     ("#E6E9EF"); // разделитель снизу
const QColor kText1   ("#22262A"); // первая строка
const QColor kSelText ("#FFFFFF"); // текст на выделении
const QColor kGrayMiddleText("#808080");   // средне-серый
const QColor kGrayLightText("#A0A0A0");   // светло-серый

}

QSize UserListItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  Q_UNUSED(index);

  QFont f1 = option.font;
  const int h = kPaddingY + QFontMetrics(f1).height() + kPaddingY;
  return { option.rect.width(), h };
}

void UserListItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  // 1) Забираем данные строго из ролей модели

  QString lineText;

  const QString login           = index.data(Qt::UserRole + 1).toString();
  const QString name            = index.data(Qt::UserRole + 2).toString();
  const bool isActive           = index.data(Qt::UserRole + 6).toBool();
  const std::int64_t bunUntil = index.data(Qt::UserRole + 8).toLongLong();

  lineText = "Имя (логин): " + name + " (" + login + ")";

  std::int64_t currentTimeStamp = getCurrentDateTimeInt();
  if (!isActive) lineText += " заблокирован.";
  else if (bunUntil > 0) {
    if (bunUntil > currentTimeStamp)
      lineText += "    Забанен до " + QString::fromStdString(formatTimeStampToString(bunUntil, true));
  }

  const bool selected = option.state & QStyle::State_Selected;
  const bool hover    = option.state & QStyle::State_MouseOver;

  QStyleOptionViewItem opt(option);
  opt.text.clear();

  painter->save();
  painter->setRenderHint(QPainter::TextAntialiasing, true);

  // фон
  if (selected)
    painter->fillRect(option.rect, kSelBg);
  else if (hover)
    painter->fillRect(option.rect, kHoverBg);
  else
    painter->fillRect(option.rect, kBg);

  // рабочая область
  QRect contentRect = option.rect.adjusted(kPaddingX, kPaddingY, -kPaddingX, -kPaddingY);

  // строки
  QFont fontLine1 = option.font;

  QColor colorLine1;

  if (!isActive || bunUntil > currentTimeStamp)
    colorLine1 = selected ? kGrayMiddleText : kGrayLightText;
    else
    colorLine1 = selected ? kSelText : kText1;

  const int h1 = QFontMetrics(fontLine1).height();
  QString line1 = QFontMetrics(fontLine1).elidedText(lineText, Qt::ElideRight, contentRect.width());

  painter->setFont(fontLine1);
  painter->setPen(colorLine1);
  painter->drawText(QRect(contentRect.left(), contentRect.top(),
                          contentRect.width(), h1),
                    Qt::AlignLeft | Qt::AlignVCenter, line1);

         // разделитель
  painter->setPen(QPen(kSep));
  painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());

  painter->restore();
}

UserListItemDelegate::UserListItemDelegate() {}



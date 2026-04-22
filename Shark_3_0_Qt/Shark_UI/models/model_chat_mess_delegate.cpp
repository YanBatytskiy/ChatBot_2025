#include "model_chat_mess_delegate.h"
#include <QApplication>
#include <QPainter>
#include <QTextLayout>
#include <QFontMetrics>
#include "system/date_time_utils.h"
#include <QPainterPath>

namespace {
constexpr int kPaddingX = 12;
constexpr int kPaddingY = 6;
constexpr int kLineSpacing = 2;

// Палитра
const QColor kBg      ("#FFFFF0"); // обычный фон
const QColor kSep     ("#E6E9EF"); // разделитель снизу
const QColor kText1   ("#800020"); //  строка c именем
const QColor kText2   ("#000000"); //  строка с текстом
}

QSize model_chat_mess_delegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  Q_UNUSED(index);

  const QString messageText  = index.data(Qt::UserRole + 1).toString();
  const QString senderLogin  = index.data(Qt::UserRole + 2).toString();
  const QString senderName   = index.data(Qt::UserRole + 3).toString();
  const std::int64_t timeStamp = index.data(Qt::UserRole + 4).toLongLong();

  const auto dateTimeStamp = formatTimeStampToString(timeStamp, true);
  const QString firstLine = senderName + " (" + senderLogin + ")          " + QString::fromStdString(dateTimeStamp);

  QFont font1 = option.font;
  QFont font2 = option.font;

  const int leftMargin     = 10;
  const int rightMargin    = 8;
  const int topPadding     = 8;
  const int bottomPadding  = 8;
  const int lineSpacing    = 2;

  const int viewWidth       = option.widget ? option.widget->width() : option.rect.width();
  const int maxBlockWidth   = static_cast<int>(0.8 * viewWidth);
const int contentMaxWidth  = qMax(0, maxBlockWidth - leftMargin - rightMargin - 2*kPaddingX);

  const int firstLineWidth = QFontMetrics(font1).horizontalAdvance(firstLine);

  QTextLayout bodyLayout;
  int height2 = 0, width2eff = 0;
  layoutBody(messageText, font2, contentMaxWidth, height2, width2eff, bodyLayout);

  const int contentWidth = qMin(qMax(firstLineWidth, width2eff), contentMaxWidth);
  const int totalHeight  = topPadding + QFontMetrics(font1).height() + lineSpacing + height2 + bottomPadding;

return QSize(leftMargin + contentWidth + rightMargin + 2*kPaddingX, totalHeight);
}


void model_chat_mess_delegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  // 1) Забираем данные строго из ролей модели
  const QString messageText   = index.data(Qt::UserRole + 1).toString();
  const QString senderLogin   = index.data(Qt::UserRole + 2).toString();
  const QString senderName    = index.data(Qt::UserRole + 3).toString();
  const std::int64_t timeStamp = index.data(Qt::UserRole + 4).toLongLong();
  const std::size_t messageId = index.data(Qt::UserRole + 5).toLongLong();

  const auto dateTimeStamp = formatTimeStampToString(timeStamp, true);
  const QString firstLine = senderName + " (" + senderLogin + ")          " + QString::fromStdString(dateTimeStamp);

  QStyleOptionViewItem opt(option);
  initStyleOption(&opt, index);

  const int leftMargin    = 10;   // отступ блока от левого края
  const int rightMargin   = 8;
  const int topPadding    = 8;    // внутренние отступы внутри блока
  const int bottomPadding = 8;
  const int lineSpacing   = 2;
  const int cornerRadius  = 8;

  QFont font1 = opt.font;
  QFont font2 = opt.font;

  const int viewWidth       = option.widget ? option.widget->width() : option.rect.width();
  const int maxBlockWidth   = static_cast<int>(0.8 * viewWidth);
  const int contentMaxWidth  = qMax(0, maxBlockWidth - leftMargin - rightMargin - 2*kPaddingX);

         // Раскладка второй строки для размеров и вывода
  QTextLayout bodyLayout;
  int height2 = 0, width2eff = 0;
  layoutBody(messageText, font2, contentMaxWidth, height2, width2eff, bodyLayout);

  const int firstLineWidth = QFontMetrics(font1).horizontalAdvance(firstLine);
  const int contentWidth   = qMin(qMax(firstLineWidth, width2eff), contentMaxWidth);

  const int blockWidth = contentWidth + 2 * kPaddingX;
  const int blockHeight = topPadding + QFontMetrics(font1).height() + lineSpacing + height2 + bottomPadding;

  const int originX = opt.rect.left() + leftMargin; // блок начинается с отступом 10
  const int originY = opt.rect.top();

painter->save();
  painter->setRenderHint(QPainter::Antialiasing, true);
painter->setRenderHint(QPainter::TextAntialiasing, true);

  // НЕ заполняем весь item — фон между блоками остаётся виден
QRectF blockRect(originX, originY + 0.5, blockWidth, blockHeight - 1);
QPainterPath blockPath;
blockPath.addRoundedRect(blockRect, cornerRadius, cornerRadius);
painter->fillPath(blockPath, kBg); // сам блок
painter->setPen(QPen(kSep));
painter->drawPath(blockPath); // обводка блока

// Первая строка — одна строка без переносов внутри блока
painter->setFont(font1);
painter->setPen(kText1);
const int firstLineHeight = QFontMetrics(font1).height();
painter->drawText(QRect(originX + kPaddingX, originY + topPadding, contentWidth,
                        firstLineHeight), // ширина = contentWidth
                  Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, firstLine);

// Вторая строка — многострочно через QTextLayout с переносами по словам
painter->setFont(font2);
painter->setPen(kText2);
const QPointF bodyOrigin(originX + kPaddingX, originY + topPadding + firstLineHeight + lineSpacing);
bodyLayout.draw(painter, bodyOrigin);

painter->restore();
}

model_chat_mess_delegate::model_chat_mess_delegate() {}

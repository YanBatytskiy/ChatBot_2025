#ifndef MODEL_CHAT_MESS_DELEGATE_H
#define MODEL_CHAT_MESS_DELEGATE_H

#include <QObject>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QTextLayout>


static inline void layoutBody(const QString& text,
                              const QFont& fontForSecondLine,
                              int maxWidthPixels,
                              int& outHeight,
                              int& outEffectiveWidth,
                              QTextLayout& outLayout) {
  outLayout.setText(text);
  outLayout.setFont(fontForSecondLine);

  QTextOption textOption;
  textOption.setWrapMode(QTextOption::WordWrap);                 // только по границам слов
  outLayout.setTextOption(textOption);

  outLayout.beginLayout();
  outHeight = 0;
  outEffectiveWidth = 0;

  while (true) {
    QTextLine line = outLayout.createLine();
    if (!line.isValid()) break;
    line.setLineWidth(maxWidthPixels);
    line.setPosition(QPointF(0, outHeight));
    outHeight += static_cast<int>(line.height());
    outEffectiveWidth = qMax(outEffectiveWidth, static_cast<int>(line.naturalTextWidth()));
  }
  outLayout.endLayout();
}


class model_chat_mess_delegate final : public QStyledItemDelegate {
  Q_OBJECT
public:

  using QStyledItemDelegate::QStyledItemDelegate;

  // explicit model_chat_mess_delegate(QObject *parent = nullptr);

  QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

  void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

  model_chat_mess_delegate();

};

#endif // MODEL_CHAT_MESS_DELEGATE_H

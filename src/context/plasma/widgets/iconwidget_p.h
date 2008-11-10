/*
 *   Copyright (C) 2007 by Aaron Seigo <aseigo@kde.org>
 *   Copyright (C) 2007 by Matt Broadstone <mbroadst@gmail.com>
 *   Copyright (C) 2006-2007 Fredrik Höglund <fredrik@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.

 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef PLASMA_ICONWIDGET_P_H
#define PLASMA_ICONWIDGET_P_H

#include <QtCore/QEvent>
#include <QtGui/QApplication>
#include <QtGui/QIcon>
#include <QtGui/QStyleOptionGraphicsItem>
#include <QtGui/QTextLayout>
#include <QtGui/QTextOption>

#include <plasma/plasma_export.h>
#include <plasma/svg.h>

#include "iconwidget.h"
#include "animator.h"

class QAction;
class QPainter;
class QTextLayout;

namespace Plasma
{

class PLASMA_EXPORT IconAction
{
public:
    IconAction(IconWidget *icon, QAction *action);

    void show();
    void hide();
    bool isVisible() const;

    int animationId() const;
    QAction *action() const;

    void paint(QPainter *painter) const;
    bool event(QEvent::Type type, const QPointF &pos);

    void setSelected(bool selected);
    bool isSelected() const;

    bool isHovered() const;
    bool isPressed() const;

    void setRect(const QRectF &rect);
    QRectF rect() const;

private:
    void rebuildPixmap();

    IconWidget *m_icon;
    QAction *m_action;
    QPixmap m_pixmap;
    QRectF m_rect;

    bool m_hovered;
    bool m_pressed;
    bool m_selected;
    bool m_visible;

    int m_animationId;
};

struct Margin
{
    qreal left, right, top, bottom;
};

class IconWidgetPrivate
{
public:
    enum MarginType {
        ItemMargin = 0,
        TextMargin,
        IconMargin,
        NMargins
    };

    enum IconWidgetState {
        NoState = 0,
        HoverState = 1,
        PressedState = 2,
        ManualPressedState = 4
    };
    Q_DECLARE_FLAGS(IconWidgetStates, IconWidgetState)

public:
    IconWidgetPrivate(IconWidget *i);
    ~IconWidgetPrivate();

    void drawBackground(QPainter *painter, IconWidgetState state);
    void drawText(QPainter *painter);
    void drawTextItems(QPainter *painter, const QStyleOptionGraphicsItem *option,
                        const QTextLayout &labelLayout, const QTextLayout &infoLayout) const;

    QPixmap decoration(const QStyleOptionGraphicsItem *option, bool useHoverEffect);
    QPointF iconPosition(const QStyleOptionGraphicsItem *option, const QPixmap &pixmap) const;

    QSizeF displaySizeHint(const QStyleOptionGraphicsItem *option, const qreal width) const;

    QBrush foregroundBrush(const QStyleOptionGraphicsItem *option) const;
    QBrush backgroundBrush(const QStyleOptionGraphicsItem *option) const;

    QString elidedText(QTextLayout &layout,
                       const QStyleOptionGraphicsItem *option,
                       const QSizeF &maxSize) const;

    QSizeF layoutText(QTextLayout &layout,
                      const QStyleOptionGraphicsItem *option,
                      const QString &text, const QSizeF &constraints) const;

    QSizeF layoutText(QTextLayout &layout, const QString &text,
                      qreal maxWidth) const;

    QRectF labelRectangle(const QStyleOptionGraphicsItem *option,
                          const QPixmap &icon, const QString &string) const;

    void layoutTextItems(const QStyleOptionGraphicsItem *option,
                         const QPixmap &icon, QTextLayout *labelLayout,
                         QTextLayout *infoLayout, QRectF *textBoundingRect) const;

    inline void setLayoutOptions(QTextLayout &layout,
                                 const QStyleOptionGraphicsItem *options) const;

    inline Qt::LayoutDirection iconDirection(const QStyleOptionGraphicsItem *option) const;

    enum {
        Minibutton = 64,
        MinibuttonHover = 128,
        MinibuttonPressed = 256
    };

    enum ActionPosition {
        TopLeft = 0,
        TopRight,
        BottomLeft,
        BottomRight,
        LastIconPosition
    };

    // Margin functions
    inline void setActiveMargins();
    void setVerticalMargin(MarginType type, qreal left, qreal right, qreal top, qreal bottom);
    void setHorizontalMargin(MarginType type, qreal left, qreal right, qreal top, qreal bottom);
    inline void setVerticalMargin(MarginType type, qreal hor, qreal ver);
    inline void setHorizontalMargin(MarginType type, qreal hor, qreal ver);
    inline QRectF addMargin(const QRectF &rect, MarginType type) const;
    inline QRectF subtractMargin(const QRectF &rect, MarginType type) const;
    inline QSizeF addMargin(const QSizeF &size, MarginType type) const;
    inline QSizeF subtractMargin(const QSizeF &size, MarginType type) const;
    inline QRectF actionRect(ActionPosition position) const;

    /**
     * update the icon's text, icon, etc. to reflect the properties of its associated action.
     */
    void syncToAction();

    IconWidget *q;
    QString text;
    QString infoText;
    Svg *iconSvg;
    QString iconSvgElement;
    QPixmap iconSvgPixmap;
    QColor textColor;
    QColor shadowColor;
    bool m_fadeIn;
    int m_hoverAnimId;
    qreal m_hoverAlpha;
    QSizeF iconSize;
    QIcon icon;
    IconWidgetStates states;
    Qt::Orientation orientation;
    int numDisplayLines;
    bool invertLayout;
    bool drawBg;
    QSizeF currentSize;
    QPointF clickStartPos;

    QList<IconAction*> cornerActions;
    QAction *action;

    Margin verticalMargin[NMargins];
    Margin horizontalMargin[NMargins];
    Margin *activeMargins;

    static const int maxDisplayLines = 5;
    static const int iconActionSize = 26;
    static const int iconActionMargin = 4;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(IconWidgetPrivate::IconWidgetStates)

// Inline methods
void IconWidgetPrivate::setLayoutOptions(QTextLayout &layout,
                                         const QStyleOptionGraphicsItem *option) const
{
    QTextOption textoption;
    textoption.setTextDirection(option->direction);
    textoption.setAlignment(Qt::AlignCenter);   // NOTE: assumption
    textoption.setWrapMode(QTextOption::WordWrap);  // NOTE: assumption as well

    layout.setFont(QApplication::font());    // NOTE: find better ways to get the font
    layout.setTextOption(textoption);
}

Qt::LayoutDirection IconWidgetPrivate::iconDirection(const QStyleOptionGraphicsItem *option) const
{
    Qt::LayoutDirection direction;

    if (invertLayout && orientation == Qt::Horizontal) {
        if (option->direction == Qt::LeftToRight) {
            direction = Qt::RightToLeft;
        } else {
            direction = Qt::LeftToRight;
        }
    } else {
        direction = option->direction;
    }

    return direction;
}

void IconWidgetPrivate::setActiveMargins()
{
    activeMargins = (orientation == Qt::Horizontal ?
            horizontalMargin : verticalMargin);
}

void IconWidgetPrivate::setVerticalMargin(MarginType type, qreal left, qreal top,
                                          qreal right, qreal bottom)
{
    verticalMargin[type].left   = left;
    verticalMargin[type].right  = right;
    verticalMargin[type].top    = top;
    verticalMargin[type].bottom = bottom;
}

void IconWidgetPrivate::setHorizontalMargin(MarginType type, qreal left, qreal top,
                                            qreal right, qreal bottom)
{
    horizontalMargin[type].left   = left;
    horizontalMargin[type].right  = right;
    horizontalMargin[type].top    = top;
    horizontalMargin[type].bottom = bottom;
}

void IconWidgetPrivate::setVerticalMargin(MarginType type, qreal horizontal, qreal vertical)
{
    setVerticalMargin(type, horizontal, vertical, horizontal, vertical);
}

void IconWidgetPrivate::setHorizontalMargin(MarginType type, qreal horizontal, qreal vertical)
{
    setHorizontalMargin(type, horizontal, vertical, horizontal, vertical);
}

QRectF IconWidgetPrivate::addMargin(const QRectF &rect, MarginType type) const
{
    Q_ASSERT(activeMargins);
    const Margin &m = activeMargins[type];
    return rect.adjusted(-m.left, -m.top, m.right, m.bottom);
}

QRectF IconWidgetPrivate::subtractMargin(const QRectF &rect, MarginType type) const
{
    Q_ASSERT(activeMargins);
    const Margin &m = activeMargins[type];
    return rect.adjusted(m.left, m.top, -m.right, -m.bottom);
}

QSizeF IconWidgetPrivate::addMargin(const QSizeF &size, MarginType type) const
{
    Q_ASSERT(activeMargins);
    const Margin &m = activeMargins[type];
    return QSizeF(size.width() + m.left + m.right, size.height() + m.top + m.bottom);
}

QSizeF IconWidgetPrivate::subtractMargin(const QSizeF &size, MarginType type) const
{
    Q_ASSERT(activeMargins);
    const Margin &m = activeMargins[type];
    return QSizeF(size.width() - m.left - m.right, size.height() - m.top - m.bottom);
}

QRectF IconWidgetPrivate::actionRect(ActionPosition position) const
{
    switch (position) {
    case TopLeft:
        return QRectF(iconActionMargin,
                      iconActionMargin,
                      iconActionSize,
                      iconActionSize);
    case TopRight:
        return QRectF(currentSize.width() - iconActionSize - iconActionMargin,
                      iconActionMargin,
                      iconActionSize,
                      iconActionSize);
    case BottomLeft:
        return QRectF(iconActionMargin,
                      currentSize.height() - iconActionSize - iconActionMargin,
                      iconActionSize,
                      iconActionSize);
    //BottomRight
    default:
        return QRectF(currentSize.width() - iconActionSize - iconActionMargin,
                      currentSize.height() - iconActionSize - iconActionMargin,
                      iconActionSize,
                      iconActionSize);
    }
}

} // Namespace

#endif


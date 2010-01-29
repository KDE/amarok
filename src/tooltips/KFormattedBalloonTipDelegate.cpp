/*******************************************************************************
 *   Copyright (C) 2008 by Fredrik Höglund <fredrik@kde.org>                   *
 *   Copyright (C) 2008 by Konstantin Heil <konst.heil@stud.uni-heidelberg.de> *
 *   Copyright (C) 2009 Oleksandr Khayrullin <saniokh@gmail.com>               *
 *                                                                             *
 *   This program is free software; you can redistribute it and/or modify      *
 *   it under the terms of the GNU General Public License as published by      *
 *   the Free Software Foundation; either version 2 of the License, or         *
 *   (at your option) any later version.                                       *
 *                                                                             *
 *   This program is distributed in the hope that it will be useful,           *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 *   GNU General Public License for more details.                              *
 *                                                                             *
 *   You should have received a copy of the GNU General Public License         *
 *   along with this program; if not, write to the                             *
 *   Free Software Foundation, Inc.,                                           *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA                *
 *******************************************************************************/

#include "KFormattedBalloonTipDelegate.h"
#include "KToolTipItem.h"
#include "KToolTip.h"
#include <QBitmap>
#include <QIcon>
#include <QLinearGradient>
#include <QSize>
#include <QTextDocument>

KFormattedBalloonTipDelegate::KFormattedBalloonTipDelegate()
{
}

KFormattedBalloonTipDelegate::~KFormattedBalloonTipDelegate()
{
}

QSize KFormattedBalloonTipDelegate::sizeHint(const KStyleOptionToolTip &option, const KToolTipItem &item) const
{
    QTextDocument doc;
    doc.setHtml(item.text());
    const QSize docSize = doc.size().toSize();

    QSize iconSize = iconResize(item, docSize);
    QSize contentSize = iconSize + docSize;

    // assure that the content height is large enough for the icon and the document
    contentSize.setHeight(doc.size().height());
    return contentSize + QSize(Border * 3, Border * 2);
}

void KFormattedBalloonTipDelegate::paint(QPainter *painter,
                                         const KStyleOptionToolTip &option,
                                         const KToolTipItem &item) const
{
    QColor color = option.palette.brush(QPalette::ToolTipBase).color();

    if (haveAlphaChannel()) {
        painter->setRenderHint(QPainter::Antialiasing);
        painter->translate(.5, .5);
    }

    painter->setBrush(color);
    QRect rect = option.rect;
    rect.setWidth(rect.width() - 1);
    rect.setHeight(rect.height() - 1);

    painter->drawRect(rect);

    QTextDocument doc;
    doc.setHtml(item.text());
    QSize docSize = doc.size().toSize();

    const QIcon icon = item.icon();
    int x = Border;
    const int y = Border;
    if (!icon.isNull()) {
        QSize iconSize = iconResize(item, docSize);
        const QPoint pos(x + option.rect.x(), y + option.rect.y());
        painter->drawPixmap(pos, icon.pixmap(iconSize));
        x += iconSize.width() + Border;
    }
    
    QPixmap bitmap(docSize);
    bitmap.fill(Qt::transparent);
    QPainter p(&bitmap);
    doc.drawContents(&p);

    const QRect docRect(QPoint(x, y), doc.size().toSize());
    painter->drawPixmap(docRect, bitmap);
}

QRegion KFormattedBalloonTipDelegate::inputShape(const KStyleOptionToolTip &option) const
{
    QBitmap bitmap(option.rect.size());
    bitmap.fill(Qt::color0);    

    QPainter p(&bitmap);
    p.setPen(QPen(Qt::color1, 1));
    p.setBrush(Qt::color1);
    p.drawRect(option.rect);

    return QRegion(bitmap);
}

QRegion KFormattedBalloonTipDelegate::shapeMask(const KStyleOptionToolTip &option) const
{
    return inputShape(option);
}

QSize KFormattedBalloonTipDelegate::iconResize(const KToolTipItem &item, const QSize &docSize) const
{
    // If there is a chance for the icon to streth the tooltip, shrink the icon so that only the width changes
    QIcon icon = item.icon();
    QSize iconSize;
    if (!icon.isNull())
    {
        iconSize = icon.actualSize(docSize);
        if (iconSize.height() > docSize.height())
        {
            iconSize.rheight() = docSize.height();
            iconSize.rwidth() *= docSize.height()/iconSize.height();
        }
    }
    else
    {
        iconSize = QSize(0, 0);
    }
    return iconSize;
}
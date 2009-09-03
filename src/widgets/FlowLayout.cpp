/****************************************************************************************
 * Copyright (c) 2004-2008 Trolltech ASA <copyright@trolltech.com>                      *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or any    *
 * later version publicly approved by Trolltech ASA (or its successor, if any) and the  *
 * KDE Free Qt Foundation.                                                              *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 *                                                                                      *
 * In addition, Trolltech gives you certain additional rights as described in the       *
 * Trolltech GPL Exception version 1.2 which can be found at                            *
 * http://www.trolltech.com/products/qt/gplexception/                                   *
 ****************************************************************************************/

#include "FlowLayout.h"

 FlowLayout::FlowLayout(QWidget *parent, int margin, int spacing)
    : QLayout(parent)
{
    setMargin(margin);
    setSpacing(spacing);
}

 FlowLayout::FlowLayout(int spacing)
{
    setSpacing(spacing);
}

 FlowLayout::~FlowLayout()
{
    QLayoutItem *item;
    while ((item = takeAt(0)))
        delete item;
}

 void FlowLayout::addItem(QLayoutItem *item)
{
    itemList.append(item);
}

 int FlowLayout::count() const
{
    return itemList.size();
}

 QLayoutItem *FlowLayout::itemAt(int index) const
{
    return itemList.value(index);
}

 QLayoutItem *FlowLayout::takeAt(int index)
{
    if (index >= 0 && index < itemList.size())
        return itemList.takeAt(index);
    else
        return 0;
}

 Qt::Orientations FlowLayout::expandingDirections() const
{
    return 0;
}

 bool FlowLayout::hasHeightForWidth() const
{
    return true;
}

 int FlowLayout::heightForWidth(int width) const
{
    int height = doLayout(QRect(0, 0, width, 0), true);
    return height;
}

 void FlowLayout::setGeometry(const QRect &rect)
{
    QLayout::setGeometry(rect);
    doLayout(rect, false);
}

 QSize FlowLayout::sizeHint() const
{
    return minimumSize();
}

 QSize FlowLayout::minimumSize() const
{
    QSize size;
    QLayoutItem *item;
    foreach (item, itemList)
        size = size.expandedTo(item->minimumSize());

    size += QSize(2*margin(), 2*margin());
    return size;
}

 int FlowLayout::doLayout(const QRect &rect, bool testOnly) const
{
    int x = rect.x();
    int y = rect.y();
    int lineHeight = 0;

    QLayoutItem *item;
    foreach (item, itemList) {
        int nextX = x + item->sizeHint().width() + spacing();
        if (nextX - spacing() > rect.right() && lineHeight > 0) {
            x = rect.x();
            y = y + lineHeight + spacing();
            nextX = x + item->sizeHint().width() + spacing();
            lineHeight = 0;
        }

        if (!testOnly)
            item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));

        x = nextX;
        lineHeight = qMax(lineHeight, item->sizeHint().height());
    }
    return y + lineHeight - rect.y();
}


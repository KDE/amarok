/*
 *   Copyright 2007 by Matias Valdenegro T. <mvaldenegro@informatica.utem.cl>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
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

#include "VBoxLayout.h"

#include "debug.h"

#include <QtCore/QList>

namespace Context
{

VBoxLayout::VBoxLayout(LayoutItem *parent)
    : BoxLayout(TopToBottom, parent),
      d(0)
{
}

VBoxLayout::~VBoxLayout()
{
}

Qt::Orientations VBoxLayout::expandingDirections() const
{
    return Qt::Vertical;
}

bool VBoxLayout::hasHeightForWidth() const
{
    return true;
}

qreal VBoxLayout::heightForWidth(qreal w) const
{
    Q_UNUSED(w);
    return qreal();
}

void VBoxLayout::setGeometry(QRectF geometry)
{
    if (!geometry.isValid() || geometry.isEmpty()) {
        kDebug() << "Invalid Geometry " << geometry;
        BoxLayout::setGeometry(geometry);
        return;
    }

    kDebug() << this << " Geometry process " << geometry << " for " << count() << " children";

    QList<QSizeF> sizes;
    QSizeF available = geometry.size() - QSizeF(2 * margin(), 2 * margin());

    // we assume all children are fixed

    for (int i = 0; i < count(); ++i) {
        LayoutItem *l = itemAt(i);
        QSizeF hint = l->sizeHint();
        sizes.insert(indexOf(l), QSizeF(available.width(), hint.height()));
        available -= QSizeF(0.0, hint.height() + spacing());
        if( available.height() < 0 )
        {
            geometry.setSize( QSizeF( geometry.size().width(), geometry.size().height() + (-1)*available.height() ) );
        }
    }

    QPointF start = geometry.topLeft();
    start += QPointF(margin(), spacing());

    for (int i = 0; i < sizes.size(); i++) {
        LayoutItem *l = itemAt(i);

        kDebug() << "Setting Geometry for child " << l << " to " << QRectF(start, sizes[i]);

        l->setGeometry(QRectF(start, sizes[i]));
        start += QPointF(0.0, sizes[i].height() + spacing());
    }

    BoxLayout::setGeometry(geometry);
}

QSizeF VBoxLayout::sizeHint() const
{
    qreal hintHeight = 0.0;
    qreal hintWidth = 0.0;

    for (int i = 0; i < count(); ++i) {
        LayoutItem *l = itemAt(i);
        QSizeF hint = l->sizeHint();

        hintWidth = qMax(hint.width(), hintWidth);
        hintHeight += hint.height() + spacing();
    }

    return QSizeF(hintWidth, hintHeight);
}

}

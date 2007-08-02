/*
 *   Copyright (C) 2007 by Matias Valdenegro T. <mvaldenegro@informatica.utem.cl>
 *   Copyright (C) 2007 by Leo Franchi <lfranchi@gmail.com> 
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

#include <KDebug>

#define DEBUG_PREFIX "VBoxLayout"

namespace Context
{

class VBoxLayout::Private
{
	public:
		Private() {}
		~Private() {}

		QRectF geometry;
		QList<LayoutItem *> childList;
};


VBoxLayout::VBoxLayout(LayoutItem *parent)
	: Plasma::VBoxLayout(parent),
	  d(0)
{
}

VBoxLayout::~VBoxLayout()
{
    delete d;
}

void VBoxLayout::setGeometry(const QRectF& geometry)
{
    DEBUG_BLOCK
    if (!geometry.isValid() || geometry.isEmpty()) {
        kDebug() << "Invalid Geometry!" << endl;
        return;
    }

    qDebug("Geometry %p : %f, %f by %f, %f", this, geometry.x(), geometry.y(), geometry.width(), geometry.height());

    QList<LayoutItem *> children;
    QList<LayoutItem *> expandingChildren;
    QList<QSizeF> sizes;
    QSizeF available = geometry.size() - QSizeF(2 * margin(), 2 * margin());

    foreach (LayoutItem *l, d->childList) {
        kDebug() << "testing layout item " << l << endl;
        if (l->expandingDirections() & Qt::Vertical) {
            expandingChildren += l;
        } else {

            children += l;
        }
    }

    foreach (LayoutItem *l, children) {
        QSizeF hint = l->sizeHint();
        sizes.insert(indexOf(l), QSizeF(available.width(), hint.height()));
        available -= QSizeF(0.0, hint.height() + spacing());
    }

    qreal expandHeight = (available.height() - ((expandingChildren.count() - 1) * spacing())) / expandingChildren.count();

    foreach (LayoutItem *l, expandingChildren) {

        sizes.insert(indexOf(l),QSizeF(available.width(), expandHeight));
    }

    QPointF start = geometry.topLeft();
    start += QPointF(margin(), spacing());

    for (int i = 0; i < sizes.size(); i++) {

        LayoutItem *l = itemAt(i);

        l->setGeometry(QRectF(start, sizes[i]));
        start += QPointF(0.0, sizes[i].height() + spacing());
    }

    Plasma::BoxLayout::setGeometry(geometry);
    /*if( sizes.size() != 0 ) // we shrink the layout
    {
        debug() << "looking up entry: " << sizes.size() - 1 << " in array of size: " << sizes.size();
        qreal bottom = sizes[ sizes.size() - 1 ].height();
        debug() << "new bottom is: " << sizeHint().height() << " and old is: " << geometry.height();
        if( sizeHint().height() < geometry.height() )
             d->geometry.setHeight( sizeHint().height() );
    }*/
}

void VBoxLayout::shrinkToMinimumSize()
{
    DEBUG_BLOCK
    QRectF geometry = this->geometry();
     debug() << "trying to shrink size from: " << geometry.height() << " to: " << sizeHint().height();
    if( sizeHint().height() < geometry.height() ) {
        geometry.setSize( sizeHint() );
        Plasma::BoxLayout::setGeometry(geometry);
    }
}

}

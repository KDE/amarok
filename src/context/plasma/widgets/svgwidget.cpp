/*
 *   Copyright 2008 by Davide Bettio <davide.bettio@kdemail.net>
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

#include "svgwidget.h"

#include <QtGui/QPainter>
#include <QtGui/QGraphicsSceneMouseEvent>

#include "svg.h"

namespace Plasma
{
class SvgWidgetPrivate
{
    public:
        SvgWidgetPrivate(Svg *s, const QString &element)
            : svg(s), elementID(element)
        {
        }

        Svg *svg;
        QString elementID;
};

SvgWidget::SvgWidget(QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : QGraphicsWidget(parent, wFlags),
      d(new SvgWidgetPrivate(0, QString()))
{
}

SvgWidget::SvgWidget(Svg *svg, const QString &elementID, QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : QGraphicsWidget(parent, wFlags),
      d(new SvgWidgetPrivate(svg, elementID))
{
}

SvgWidget::~SvgWidget()
{
    delete d;
}

void SvgWidget::mouseReleaseEvent ( QGraphicsSceneMouseEvent * event  )
{
    if (receivers(SIGNAL(clicked(Qt::MouseButton)))){
        emit clicked(event->button());
    }else{
        event->accept();
    }
}

void SvgWidget::setSvg(Svg *svg)
{
    d->svg = svg;
}

Svg *SvgWidget::svg() const
{
    return d->svg;
}

void SvgWidget::setElementID(const QString &elementID)
{
    d->elementID = elementID;
}

QString SvgWidget::elementID() const
{
    return d->elementID;
}

void SvgWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (d->svg){
        d->svg->paint(painter, boundingRect(), d->elementID);
    }
}

} // Plasma namespace

#include "svgwidget.moc"

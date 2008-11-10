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

#ifndef PLASMA_SVGWIDGET_H
#define PLASMA_SVGWIDGET_H

#include <QtGui/QGraphicsWidget>

#include <plasma/plasma_export.h>

#include <plasma/plasma.h>

namespace Plasma
{

class Svg;
class SvgWidgetPrivate;

class PLASMA_EXPORT SvgWidget : public QGraphicsWidget
{
    Q_OBJECT

    Q_PROPERTY(Svg *svg READ svg WRITE setSvg)
    Q_PROPERTY(QString elementID READ elementID WRITE setElementID)

    public:
        SvgWidget(QGraphicsItem *parent = 0, Qt::WindowFlags wFlags = 0);
        SvgWidget(Svg *svg, const QString & elementID = QString(),
                  QGraphicsItem *parent = 0, Qt::WindowFlags wFlags = 0);
        virtual ~SvgWidget();

        virtual void mouseReleaseEvent ( QGraphicsSceneMouseEvent * event );

        void setSvg(Svg *svg);
        Svg *svg() const;

        void setElementID(const QString &elementID);
        QString elementID() const;

    Q_SIGNALS:
        void clicked(Qt::MouseButton);

    protected:
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    private:
        SvgWidgetPrivate * const d;
};

} // Plasma namespace

#endif // multiple inclusion guard

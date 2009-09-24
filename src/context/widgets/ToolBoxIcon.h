/****************************************************************************************
 * Copyright (c) 2008 William Viana Soares <vianasw@gmail.com>                          *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef TOOLBOX_ICON_H
#define TOOLBOX_ICON_H

#include "amarok_export.h"

#include <plasma/widgets/iconwidget.h>

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QTextLine>

class QPainterPath;

class AMAROK_EXPORT ToolBoxIcon: public Plasma::IconWidget
{
    Q_OBJECT

public:
    explicit ToolBoxIcon( QGraphicsItem *parent = 0, const float opacity = 0.8 );
    ~ToolBoxIcon();
    
    /**
     * reimplemented from Plasma::Icon
     */
    QPainterPath shape() const;

    QRectF boundingRect() const;

    /**
     * reimplemented from Plasma::Icon
     */
    void setText( const QString &text );

    QString text() const;

    void setBrush( const QBrush& );
    
protected:
    void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0 );
    void hoverEnterEvent( QGraphicsSceneHoverEvent *event );
    void hoverLeaveEvent( QGraphicsSceneHoverEvent *event );
    void mousePressEvent( QGraphicsSceneMouseEvent *event );

Q_SIGNALS:
    void appletChosen( const QString &pluginName );
    
private slots:
    void animateHighlight( qreal progress );
    void mousePressed( bool pressed );
    
private:
    bool m_hovering;

    const qreal m_baseOpacity;
    qreal m_animOpacity;
    int m_animHighlightId;

    QGraphicsSimpleTextItem *m_text;
    QBrush m_defaultTextBrush;
};

#endif


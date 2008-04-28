/***************************************************************************
 *   Copyright (c) 2008  Jeff Mitchell <mitchell@kde.org>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef POPUPDROPPERITEM_H
#define POPUPDROPPERITEM_H

#include <QtSvg/QGraphicsSvgItem>
#include <QString>
#include <QTimer>
#include <QFont>

class QAction;
class QDropEvent;
class QGraphicsTextItem;
class QGraphicsView;
class QSvgRenderer;
class PopupDropper;
class PopupDropperItemPrivate;

class PopupDropperItem : public QGraphicsSvgItem
{
    Q_OBJECT

    Q_PROPERTY( QAction* action READ action WRITE setAction )
    Q_PROPERTY( QString text READ text WRITE setText )
    Q_PROPERTY( QFont font READ font WRITE setFont )
    Q_PROPERTY( QGraphicsTextItem* textItem READ textItem WRITE setTextItem )
    Q_PROPERTY( QSvgRenderer* sharedRenderer READ sharedRenderer WRITE setSharedRenderer )
    Q_PROPERTY( QString elementId READ elementId WRITE setElementId )
    Q_PROPERTY( int hoverMsecs READ hoverMsecs WRITE setHoverMsecs )

public:
    PopupDropperItem( QGraphicsItem *parent = 0 );
    PopupDropperItem( const QString &file, QGraphicsItem *parent = 0 );
    virtual ~PopupDropperItem();

    void show();

    QAction* action() const;
    void setAction( QAction *action );

    QString text() const;
    void setText( const QString &text );
    QFont font() const;
    void setFont( const QFont &font );

    QGraphicsTextItem* textItem() const;
    void setTextItem( QGraphicsTextItem* textItem );
    void reposTextItem();

    QSvgRenderer* sharedRenderer() const;
    void setSharedRenderer( QSvgRenderer *renderer );
    QString elementId() const;
    void setElementId( const QString &id );

    int hoverMsecs() const;
    void setHoverMsecs( const int msecs );
    void startHoverTimer();
    void stopHoverTimer();

    bool operator<( const PopupDropperItem &other ) const;

public slots:
    virtual void dropped( QDropEvent *event );
    virtual void hoverTimeout();

private:
    friend class PopupDropperItemPrivate;
    PopupDropperItemPrivate* const d;

signals:
    void dropEvent( QDropEvent *event );
};

#endif

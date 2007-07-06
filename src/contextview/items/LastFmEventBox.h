/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef LASTFM_EVENT_BOX_H
#define LASTFM_EVENT_BOX_H

#include "../contextbox.h"

#include <kurl.h>

using namespace Context;

typedef struct {
    QString title;
    QString description;
    QString location;
    QString city;
    KUrl link;
    QString date;
} LastFmEvent;

// we need to define LastFmEvent before including this
#include "LastFmEventItem.h"

class LastFmEventMember : public QObject, public QGraphicsRectItem
{
    Q_OBJECT
public:
    LastFmEventMember( QGraphicsItem * parent, LastFmEvent event, qreal top );
    
    void ensureWidthFits( qreal width );
private:
    QString shrinkText( QString text, const qreal length );
    
    QString title;
    QGraphicsTextItem* m_left;
    QGraphicsTextItem* m_right;
};


class LastFmEventBox : public ContextBox
{
    Q_OBJECT
    
public:
    LastFmEventBox( QGraphicsItem *parent = 0, QGraphicsScene *scene = 0 );
    
    void ensureWidthFits( const qreal width );
    
    void setEvents( QList< LastFmEvent >* events );
    
    void clearContents();
    QGraphicsRectItem* getContentBox() { return m_contentRect; }
    void setContentHeight( qreal height );
private:
    QList< LastFmEventMember* > m_events;
};

#endif 

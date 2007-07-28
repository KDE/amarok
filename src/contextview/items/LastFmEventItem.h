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

#ifndef LASTFM_EVENT_ITEM_H
#define LASTFM_EVENT_ITEM_H

#include "ContextItem.h"
#include "../ContextObserver.h"
#include "LastFmEventBox.h"

#include "kio/job.h"

#include <QDomDocument>

class QGraphicsRectItem;
class LastFmEventBox;

using namespace Context; 

class LastFmEventItem : public ContextItem, public ContextObserver
{
    Q_OBJECT

public:
    LastFmEventItem();
    
    void message( const QString& msg );
    const QString name() { return "lastfm events"; }
    const QString shownDuring() { return "home"; }
    void enable() { m_enabled = true; }
    void disable() { m_enabled = false; }
    
    void showFriendEvents();
    void showSysEvents();
    void showUserEvents();
    
private:
    
    QList< LastFmEvent >* parseFeed( QString content );
    qreal generateContents( QList< LastFmEvent > items, QGraphicsRectItem* parent );
        
    QString getCached( QString path ); // returns the contents of the cached RSS feed. if
                                       //it is not cached an empty string is returned
    LastFmEvent parseTitle( QString title );
    
        
    LastFmEventBox* m_friendBox;
    LastFmEventBox* m_sysBox;
    LastFmEventBox* m_userBox;
    
    KJob* m_friendJob;
    KJob* m_sysJob;
    KJob* m_userJob;
    
    bool m_enabled;
    
    bool m_friendVisible;
    bool m_sysVisible;
    bool m_userVisible;
    
private slots:
    
    void friendResult( KJob* kob );
    void sysResult( KJob* job );
    void userResult( KJob* job );

};

#endif

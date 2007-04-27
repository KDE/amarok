/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/ 


#ifndef AMAROKMAGNATUENTREEITEM_H
#define AMAROKMAGNATUENTREEITEM_H

#include "../servicemodelitembase.h"
#include "simpleservicetypes.h"

#include "databasehandlerbase.h"

#include "kurl.h"

#include <QList>


enum {SERVICE_ITEM_ROOT, SERVICE_ITEM_ARTIST, SERVICE_ITEM_ALBUM, SERVICE_ITEM_TRACK};

union contentTypeUnion
{
    SimpleServiceArtist * artistValue;
    SimpleServiceAlbum * albumValue;
    SimpleServiceTrack * trackValue;
};

class DatabaseDrivenContentItem : public ServiceModelItemBase
{
public:
    DatabaseDrivenContentItem( SimpleServiceArtist *content, const QString &genre, DatabaseDrivenContentItem *parent, DatabaseHandlerBase * dbHandler );
    DatabaseDrivenContentItem( SimpleServiceAlbum *content, const QString &genre, DatabaseDrivenContentItem *parent, DatabaseHandlerBase * dbHandler );
    DatabaseDrivenContentItem( SimpleServiceTrack *content, const QString &genre, DatabaseDrivenContentItem *parent, DatabaseHandlerBase * dbHandler );
    DatabaseDrivenContentItem( const QString &genre, DatabaseHandlerBase * dbHandler );


    virtual ~DatabaseDrivenContentItem();

    DatabaseDrivenContentItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    /*DatabaseDrivenContentItem * parent();*/
    int getType();
    QList<ServiceModelItemBase*> getChildItems() const;
    bool hasChildren () const;
    contentTypeUnion getContentUnion ( );
    QString getUrl();


    int prePopulate() const;
    void populate() const;

private:

    mutable QList<DatabaseDrivenContentItem*> m_prefetchedItems;

    DatabaseHandlerBase * m_dbHandler;

    contentTypeUnion m_content; 
    QString m_genre;

    int m_type;
    mutable bool m_hasPopulatedChildItems;

    /*DatabaseDrivenContentItem *m_parent;*/

    void populateChildItems() const;
};

#endif  

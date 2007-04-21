/***************************************************************************
 *   Copyright (c) 2006, 2007                                              *
 *        Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                   *
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
#include "magnatunetypes.h"

#include "kurl.h"

#include <QList>


enum {MAGNATUNE_ROOT, MAGNATUNE_ARTIST, MAGNATUNE_ALBUM, MAGNATUNE_TRACK};

union contentTypeUnion
{
    MagnatuneArtist * artistValue;
    MagnatuneAlbum * albumValue;
    MagnatuneTrack * trackValue;
};

class MagnatuneContentItem : public ServiceModelItemBase
{
public:
    MagnatuneContentItem( MagnatuneArtist content, const QString &genre, MagnatuneContentItem *parent );
    MagnatuneContentItem( MagnatuneAlbum content, const QString &genre, MagnatuneContentItem *parent );
    MagnatuneContentItem( MagnatuneTrack content, const QString &genre, MagnatuneContentItem *parent );
    MagnatuneContentItem( const QString &genre );



    ~MagnatuneContentItem();

    MagnatuneContentItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    /*MagnatuneContentItem * parent();*/
    int getType();
    QList<ServiceModelItemBase*> getChildItems() const;
    bool hasChildren () const;
    contentTypeUnion getContentUnion ( );
    QString getUrl();

    void populate();

private:

    /*mutable QList<MagnatuneContentItem*> m_childItems;*/

    contentTypeUnion m_content; 
    QString m_genre;

    int m_type;
    mutable bool m_hasPopulatedChildItems;

    /*MagnatuneContentItem *m_parent;*/

    void populateChildItems() const;
};

#endif  

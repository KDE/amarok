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
#include "jamendotypes.h"

#include "kurl.h"

#include <QList>


enum {JAMENDO_ROOT, JAMENDO_ARTIST, JAMENDO_ALBUM, JAMENDO_TRACK};

union contentTypeUnion
{
    JamendoArtist * artistValue;
    JamendoAlbum * albumValue;
    JamendoTrack * trackValue;
};

class JamendoContentItem : public ServiceModelItemBase
{
public:
    JamendoContentItem( JamendoArtist content, const QString &genre, JamendoContentItem *parent );
    JamendoContentItem( JamendoAlbum content, const QString &genre, JamendoContentItem *parent );
    JamendoContentItem( JamendoTrack content, const QString &genre, JamendoContentItem *parent );
    JamendoContentItem( const QString &genre );



    ~JamendoContentItem();

    JamendoContentItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    /*JamendoContentItem * parent();*/
    int getType();
    QList<ServiceModelItemBase*> getChildItems() const;
    bool hasChildren () const;
    contentTypeUnion getContentUnion ( );
    QString getUrl();

    void populate();

private:

    /*mutable QList<JamendoContentItem*> m_childItems;*/

    contentTypeUnion m_content; 
    QString m_genre;

    int m_type;
    mutable bool m_hasPopulatedChildItems;

    /*JamendoContentItem *m_parent;*/

    void populateChildItems() const;
};

#endif  

/*
Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public License
along with this library; see the file COPYING.LIB.  If not, write to
the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
Boston, MA 02110-1301, USA.
*/


#ifndef AMAROKMAGNATUENTREEITEM_H
#define AMAROKMAGNATUENTREEITEM_H

#include "magnatunetypes.h"

#include <QList>


enum {MAGNATUNE_ROOT, MAGNATUNE_ARTIST, MAGNATUNE_ALBUM, MAGNATUNE_TRACK};

union contentTypeUnion
{
    MagnatuneArtist * artistValue;
    MagnatuneAlbum * albumValue;
    MagnatuneTrack * trackValue;
};

class MagnatuneContentItem
{
public:
    MagnatuneContentItem( MagnatuneArtist content, MagnatuneContentItem *parent );
    MagnatuneContentItem( MagnatuneAlbum content, MagnatuneContentItem *parent );
    MagnatuneContentItem( MagnatuneTrack content, MagnatuneContentItem *parent );
    MagnatuneContentItem( );



    ~MagnatuneContentItem();

    MagnatuneContentItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    MagnatuneContentItem * parent();
    int getType();
    QList<MagnatuneContentItem*> GetChildItems() const;

private:

    mutable QList<MagnatuneContentItem*> m_childItems;

    contentTypeUnion m_content; 

    int m_type;
    mutable bool m_hasPopulatedChildItems;

    MagnatuneContentItem *m_parent;

    void populateChildItems() const;
};

#endif  

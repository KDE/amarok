/***************************************************************************
                          playlistitem.h  -  description
                             -------------------
    begin                : Die Dez 3 2002
    copyright            : (C) 2002 by Mark Kretschmann
    email                : markey@web.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include <klistview.h> //baseclass
#include <kurl.h>      //stack allocated

class QColorGroup;
class QListViewItem;
class QPainter;
class MetaBundle;
class PlaylistWidget;

class PlaylistItem : public KListViewItem
{
    public:
        PlaylistItem( PlaylistWidget*, QListViewItem*, const KURL&, const QString& = "", const int length = 0 );

        QString text( int column ) const;
        void setText( const MetaBundle& );
        void setText( int, const QString& );

        PlaylistWidget *listView() const { return (PlaylistWidget *)KListViewItem::listView(); }

        MetaBundle metaBundle();
        const QString trackName() const { return KListViewItem::text( 0 ); }
        const QString title() const { return KListViewItem::text( 1 ); }
        const KURL &url() const { return m_url; }

        bool corruptFile;

    private:
        int  compare( QListViewItem*, int, bool ) const;
        void paintCell( QPainter*, const QColorGroup&, int, int, int );

        const KURL m_url;
};

#endif

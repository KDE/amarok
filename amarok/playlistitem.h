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

class QColor;
class QColorGroup;
class QListViewItem;
class QPainter;
class QString;
class MetaBundle;

class PlayerApp;
extern PlayerApp *pApp;


class PlaylistItem : public KListViewItem
{
    public:
        PlaylistItem( QListView *, QListViewItem *, const KURL &, const MetaBundle * = 0 );
        ~PlaylistItem();

        MetaBundle metaBundle() const;
        void setMeta( const MetaBundle& );
        void writeTag( const QString&, int );

        const QString trackName() const { return text( 0 ); }
        const QString length( uint = 0 ) const; //Return track length as mm:ss
        const KURL    &url() const { return m_url; }

        //FIXME move to PlaylistWidget?
        static PlaylistItem *GlowItem;

    private:
        int  compare( QListViewItem*, int, bool ) const;
        void paintCell( QPainter*, const QColorGroup&, int, int, int );

        const KURL m_url;
};

#endif

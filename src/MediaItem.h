/*  Copyright (C) 2005-2007 Jeff Mitchell <kde-dev@emailgoeshere.com>
    (c) 2004 Christian Muehlhaeuser <chris@chris.de>
    (c) 2005-2006 Martin Aumueller <aumuell@reserv.at>
    (c) 2005 Seb Ruiz <me@sebruiz.net>
    (c) 2006 T.R.Shashwath <trshash84@gmail.com>

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


#ifndef AMAROK_MEDIAITEM_H
#define AMAROK_MEDIAITEM_H

#include <k3listview.h>

#include "amarok_export.h"

class MediaDevice;
class MetaBundle;

class AMAROK_EXPORT MediaItem : public K3ListViewItem
{
    public:
        MediaItem( Q3ListView* parent );
        MediaItem( Q3ListViewItem* parent );
        MediaItem( Q3ListView* parent, Q3ListViewItem* after );
        MediaItem( Q3ListViewItem* parent, Q3ListViewItem* after );
        void init();
        virtual ~MediaItem();

        MediaItem *lastChild() const;

        virtual KUrl url() const;
        const MetaBundle *bundle() const;
        void setBundle( MetaBundle *bundle );

        enum Type { UNKNOWN, ARTIST, ALBUM, TRACK, PODCASTSROOT, PODCASTCHANNEL,
                    PODCASTITEM, PLAYLISTSROOT, PLAYLIST, PLAYLISTITEM, INVISIBLEROOT,
                    INVISIBLE, STALEROOT, STALE, ORPHANEDROOT, ORPHANED, DIRECTORY };

        enum Flags { Failed=1, BeginTransfer=2, StopTransfer=4, Transferring=8, SmartPlaylist=16 };

        void setType( Type type );
        void setFailed( bool failed=true );
        Type type() const { return m_type; }
        MediaItem *findItem(const QString &key, const MediaItem *after=0) const;
        const QString &data() const { return m_data; }
        void setData( const QString &data ) { m_data = data; }

        virtual bool isLeafItem()     const;        // A leaf node of the tree
        virtual bool isFileBacked()   const;      // Should the file be deleted of the device when removed
        virtual QDateTime playTime()  const { return QDateTime(); }
        virtual int  played()         const { return 0; }
        virtual int  recentlyPlayed() const { return 0; } // no of times played on device since last sync
        virtual void setPlayCount( int ) {}
        virtual int  rating()         const { return 0; } // rating on device, normalized to 100
        virtual void setRating( int /*rating*/ ) {}
        virtual bool ratingChanged()  const { return false; }
        virtual void setLastPlayed( uint ) {}
        virtual void syncStatsFromPath( const QString &path );
        virtual long size()           const;
        virtual MediaDevice *device() const { return m_device; }
        virtual bool listened()       const { return m_listened; }
        virtual void setListened( bool listened=true ) { m_listened = listened; }

        int compare( Q3ListViewItem *i, int col, bool ascending ) const;
        int flags() const { return m_flags; }
        void createToolTip();

        void paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align );

        //attributes:
        int             m_order;
        Type            m_type;
        QString         m_playlistName;
        QString         m_data;
        MediaDevice    *m_device;
        int             m_flags;
        bool            m_listened;

        static QPixmap *s_pixUnknown;
        static QPixmap *s_pixRootItem;
        static QPixmap *s_pixFile;
        static QPixmap *s_pixArtist;
        static QPixmap *s_pixAlbum;
        static QPixmap *s_pixPlaylist;
        static QPixmap *s_pixPodcast;
        static QPixmap *s_pixTrack;
        static QPixmap *s_pixInvisible;
        static QPixmap *s_pixStale;
        static QPixmap *s_pixOrphaned;
        static QPixmap *s_pixDirectory;
        static QPixmap *s_pixTransferFailed;
        static QPixmap *s_pixTransferBegin;
        static QPixmap *s_pixTransferEnd;

    private:
        mutable MetaBundle *m_bundle;
};

#endif /*AMAROK_MEDIAITEM_H*/



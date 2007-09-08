/*  Copyright (C) 2005-2007 Jeff Mitchell <kde-dev@emailgoeshere.com>
    (c) 2004 Christian Muehlhaeuser <chris@chris.de>
    (c) 2005-2006 Martin Aumueller <aumuell@reserv.at>
    (c) 2005 Seb Ruiz <ruiz@kde.org>  
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


#ifndef AMAROK_MEDIADEVICEMETA_H
#define AMAROK_MEDIADEVICEMETA_H

#include "meta/meta.h"
#include "amarok_export.h"

#include <QStringList>
#include <k3listview.h>

class MediaDevice;

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

namespace Meta
{

class MediaDeviceTrack;
class MediaDeviceAlbum;
class MediaDeviceArtist;
class MediaDeviceGenre;

typedef KSharedPtr<MediaDeviceTrack> MediaDeviceTrackPtr;
typedef KSharedPtr<MediaDeviceArtist> MediaDeviceArtistPtr;
typedef KSharedPtr<MediaDeviceAlbum> MediaDeviceAlbumPtr;
typedef KSharedPtr<MediaDeviceGenre> MediaDeviceGenrePtr;


typedef QList<MediaDeviceTrackPtr > MediaDeviceTrackList;
typedef QList<MediaDeviceArtistPtr > MediaDeviceArtistList;
typedef QList<MediaDeviceAlbumPtr > MediaDeviceAlbumList;
typedef QList<MediaDeviceGenrePtr > MediaDeviceGenreList;

class MediaDeviceTrack : public Meta::Track
{
    public:
        //Give this a displayable name as some services has terrible names for their streams
        MediaDeviceTrack( const QString & name );

        //create track based on an sql query result
        MediaDeviceTrack( const QStringList & resultRow );
        virtual ~MediaDeviceTrack();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual KUrl playableUrl() const;
        virtual QString url() const;
        virtual QString prettyUrl() const;

        virtual bool isPlayable() const;
        virtual bool isEditable() const;

        virtual AlbumPtr album() const;
        virtual ArtistPtr artist() const;
        virtual GenrePtr genre() const;
        virtual ComposerPtr composer() const;
        virtual YearPtr year() const;

        virtual void setAlbum ( const QString &newAlbum );
        virtual void setArtist ( const QString &newArtist );
        virtual void setGenre ( const QString &newGenre );
        virtual void setComposer ( const QString &newComposer );
        virtual void setYear ( const QString &newYear );

        virtual void setTitle( const QString &newTitle );

        virtual QString comment() const;
        virtual void setComment ( const QString &newComment );

        virtual double score() const;
        virtual void setScore ( double newScore );

        virtual int rating() const;
        virtual void setRating ( int newRating );

        virtual int length() const;

        virtual int filesize() const;
        virtual int sampleRate() const;
        virtual int bitrate() const;

        virtual int trackNumber() const;
        virtual void setTrackNumber ( int newTrackNumber );

        virtual int discNumber() const;
        virtual void setDiscNumber ( int newDiscNumber );

        virtual uint lastPlayed() const;
        virtual int playCount() const;

        virtual QString type() const;

        virtual void beginMetaDataUpdate() {}    //read only
        virtual void endMetaDataUpdate() {}      //read only
        virtual void abortMetaDataUpdate() {}    //read only

        //MediaDeviceTrack specific methods

        void setAlbum( Meta::AlbumPtr album );
        void setArtist( Meta::ArtistPtr artist );
        void setComposer( Meta::ComposerPtr composer );
        void setGenre( Meta::GenrePtr genre );
        void setYear( Meta::YearPtr year );

        void setLength( int length );

    private:
        ArtistPtr m_artist;
        AlbumPtr m_album;
        GenrePtr m_genre;
        ComposerPtr m_composer;
        YearPtr m_year;

        QString m_name;
        int m_trackNumber;
        int m_length;
        QString m_displayUrl;
        QString m_playableUrl;
        QString m_albumName;
        QString m_artistName;

        QString m_type;
};

class MediaDeviceArtist : public Meta::Artist
{
    public:

        MediaDeviceArtist( const QStringList & resultRow );
        MediaDeviceArtist( const QString & name );
        virtual ~MediaDeviceArtist();

        virtual QString name() const;
        virtual QString prettyName() const;
        virtual void setTitle( const QString &title );
        virtual TrackList tracks();

        //MediaDeviceArtist specific methods

        void addTrack( TrackPtr track );

    private:
        QString m_name;
        QString m_description;
        TrackList m_tracks;

};

class MediaDeviceAlbum : public Meta::Album
{
    public:
        MediaDeviceAlbum( const QStringList & resultRow );
        MediaDeviceAlbum( const QString & name  );
        virtual ~MediaDeviceAlbum();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual bool isCompilation() const;
        virtual bool hasAlbumArtist() const;
        virtual ArtistPtr albumArtist() const;
        virtual TrackList tracks();

        //MediaDeviceAlbum specific methods

        void addTrack( TrackPtr track );
        void setAlbumArtist( ArtistPtr artist );
        void setIsCompilation( bool compilation );

        void setDescription( const QString &description );
        QString description( ) const;
        void setId( int id );
        int id( ) const;
        void setArtistId( int artistId );
        int artistId( ) const;
        void setArtistName( const QString &name );
        QString artistName() const;
        void setTitle( const QString &title );

    private:
        int m_id;
        QString m_name;
        TrackList m_tracks;
        bool m_isCompilation;
        ArtistPtr m_albumArtist;
        QString m_description;
        int m_artistId;
        QString m_artistName;
};

class MediaDeviceGenre : public Meta::Genre
{
    public:
        MediaDeviceGenre( const QString &name );
        MediaDeviceGenre( const QStringList &row );
        virtual ~MediaDeviceGenre();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        //MediaDeviceGenre specific methods
        void addTrack( TrackPtr track );
        void setName( const QString &name );
        int albumId();
        void setAlbumId( int albumId );

    private:
        int m_albumId;
        QString m_name;
        TrackList m_tracks;
};


}


#endif /*AMAROK_MEDIADEVICEMETA_H*/



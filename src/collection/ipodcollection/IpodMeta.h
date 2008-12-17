/* This file is part of the KDE project

   Note: Mostly taken from Daap code:
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef IPODMETA_H
#define IPODMETA_H

#include "Debug.h"
#include "Meta.h"

extern "C" {
#include <gpod/itdb.h>
}

#include <QMultiMap>

class IpodCollection;
class PopupDropperAction;

namespace Meta
{

class IpodTrack;
class IpodAlbum;
class IpodArtist;
class IpodGenre;
class IpodComposer;
class IpodYear;

typedef KSharedPtr<IpodTrack> IpodTrackPtr;
typedef KSharedPtr<IpodArtist> IpodArtistPtr;
typedef KSharedPtr<IpodAlbum> IpodAlbumPtr;
typedef KSharedPtr<IpodGenre> IpodGenrePtr;
typedef KSharedPtr<IpodComposer> IpodComposerPtr;
typedef KSharedPtr<IpodYear> IpodYearPtr;

typedef QMultiMap<IpodArtistPtr, IpodTrackPtr> IpodArtistMap;
typedef QMultiMap<IpodAlbumPtr, IpodTrackPtr> IpodAlbumMap;
typedef QMultiMap<IpodGenrePtr, IpodTrackPtr> IpodGenreMap;
typedef QMultiMap<IpodComposerPtr, IpodTrackPtr> IpodComposerMap;
typedef QMultiMap<IpodYearPtr, IpodTrackPtr> IpodYearMap;

class IpodTrack : public Meta::Track
{

    public:
        IpodTrack( IpodCollection *collection );
        virtual ~IpodTrack();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual KUrl playableUrl() const;
        virtual QString uidUrl() const;
        virtual QString prettyUrl() const;

        virtual bool isPlayable() const;
        virtual bool isEditable() const;

        virtual AlbumPtr album() const;
        virtual ArtistPtr artist() const;
        virtual GenrePtr genre() const;
        virtual ComposerPtr composer() const;
        virtual YearPtr year() const;

        Itdb_Track* getIpodTrack() const;
        void setIpodTrack ( Itdb_Track *ipodtrack );

	QList<Itdb_Playlist*> getIpodPlaylists() const;
	void addIpodPlaylist ( Itdb_Playlist *ipodplaylist );

        virtual void setAlbum ( const QString &newAlbum );
        virtual void setArtist ( const QString &newArtist );
        virtual void setGenre ( const QString &newGenre );
        virtual void setComposer ( const QString &newComposer );
        virtual void setYear ( const QString &newYear );

        virtual QString title() const;
        virtual void setTitle( const QString &newTitle );

        virtual QString comment() const;
        virtual void setComment ( const QString &newComment );

        virtual double score() const;
        virtual void setScore ( double newScore );

        virtual int rating() const;
        virtual void setRating ( int newRating );

        virtual int length() const;

        void setFileSize( int newFileSize );
        virtual int filesize() const;

        virtual int bitrate() const;
        virtual void setBitrate( int newBitrate );

        virtual int sampleRate() const;
        virtual void setSamplerate( int newSamplerate );

        virtual float bpm() const;
        virtual void setBpm( float newBpm );

        virtual int trackNumber() const;
        virtual void setTrackNumber ( int newTrackNumber );

        virtual int discNumber() const;
        virtual void setDiscNumber ( int newDiscNumber );

        virtual uint lastPlayed() const;

        virtual int playCount() const;
        void setPlayCount( const int newCount );

        virtual QString type() const;

        virtual void beginMetaDataUpdate() { DEBUG_BLOCK }    //read only
        virtual void endMetaDataUpdate();
        virtual void abortMetaDataUpdate() { DEBUG_BLOCK }    //read only

//        virtual void subscribe ( Observer *observer );
//        virtual void unsubscribe ( Observer *observer );

        virtual bool inCollection() const;
        virtual Amarok::Collection* collection() const;

	virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const;
	virtual Meta::Capability* asCapabilityInterface( Meta::Capability::Type type );

        //IpodTrack specific methods
        IpodAlbumPtr ipodAlbum() const { return m_album; }
        
        // These methods are for MemoryMatcher to use
        void setAlbum( IpodAlbumPtr album );
        void setArtist( IpodArtistPtr artist );
        void setComposer( IpodComposerPtr composer );
        void setGenre( IpodGenrePtr genre );
        void setYear( IpodYearPtr year );

        void setType( const QString & type );

        // These methods are for IpodTrack-specific usage
        // NOTE: these methods/data may turn out to be unneeded
        IpodArtistMap getIpodArtistMap() const { return m_ipodArtistMap; }
        IpodAlbumMap getIpodAlbumMap() const { return m_ipodAlbumMap; }
        IpodGenreMap getIpodGenreMap() const { return m_ipodGenreMap; }
        IpodComposerMap getIpodComposerMap() const { return m_ipodComposerMap; }
        IpodYearMap getIpodYearMap() const { return m_ipodYearMap; }

        void setIpodArtistMap( const IpodArtistMap &ipodArtistMap ) { m_ipodArtistMap = ipodArtistMap; }
        void setIpodAlbumMap( const IpodAlbumMap &ipodAlbumMap ) { m_ipodAlbumMap = ipodAlbumMap; }
        void setIpodGenreMap( const IpodGenreMap &ipodGenreMap ) { m_ipodGenreMap = ipodGenreMap; }
        void setIpodComposerMap( const IpodComposerMap &ipodComposerMap ) { m_ipodComposerMap = ipodComposerMap; }
        void setIpodYearMap( const IpodYearMap &ipodYearMap ) { m_ipodYearMap = ipodYearMap; }

    
        void setLength( int length );
	void setPlayableUrl( QString url ) { m_playableUrl = url; }

    private:
        IpodCollection *m_collection;

        IpodArtistPtr m_artist;
        IpodAlbumPtr m_album;
        IpodGenrePtr m_genre;
        IpodComposerPtr m_composer;
        IpodYearPtr m_year;

        // For IpodTrack-specific use

        IpodArtistMap m_ipodArtistMap;
        IpodAlbumMap m_ipodAlbumMap;
        IpodGenreMap m_ipodGenreMap;
        IpodComposerMap m_ipodComposerMap;
        IpodYearMap m_ipodYearMap;

        Itdb_Track *m_ipodtrack;
	QList<Itdb_Playlist*> m_ipodplaylists;

        QImage m_image;

        QString m_comment;
        QString m_name;
        QString m_type;
        int m_bitrate;
        int m_filesize;
        int m_length;
        int m_discNumber;
        int m_samplerate;
        int m_trackNumber;
        int m_playCount;
        int m_rating;
        float m_bpm;
        QString m_displayUrl;
        QString m_playableUrl;
};

class IpodArtist : public Meta::Artist
{
    public:
        IpodArtist( const QString &name );
        virtual ~IpodArtist();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        virtual AlbumList albums();

        //IpodArtist specific methods
        void addTrack( IpodTrackPtr track );
        void remTrack( IpodTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class IpodAlbum : public Meta::Album
{
    public:
        IpodAlbum( const QString &name );
        virtual ~IpodAlbum();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual bool isCompilation() const;
        virtual bool hasAlbumArtist() const;
        virtual ArtistPtr albumArtist() const;
        virtual TrackList tracks();

        virtual QPixmap image( int size = 1 );
        virtual bool canUpdateImage() const;
        virtual void setImage( const QImage &image );
        virtual bool hasImage( int size = 1 ) const { Q_UNUSED( size ); return m_hasCover; }

        //IpodAlbum specific methods
        
        void addTrack( IpodTrackPtr track );
        void remTrack( IpodTrackPtr track );
        void setAlbumArtist( IpodArtistPtr artist );
        void setIsCompilation( bool compilation );

        void setImagePath( const QString &path );

    private:
        QString m_name;
        QString m_coverPath;
        TrackList m_tracks;
        bool m_isCompilation;
        bool m_hasCover;
        QImage m_image;
        IpodArtistPtr m_albumArtist;
};

class IpodGenre : public Meta::Genre
{
    public:
        IpodGenre( const QString &name );
        virtual ~IpodGenre();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        //IpodGenre specific methods
        void addTrack( IpodTrackPtr track );
        void remTrack( IpodTrackPtr track );
    private:
        QString m_name;
        TrackList m_tracks;
};

class IpodComposer : public Meta::Composer
{
    public:
        IpodComposer( const QString &name );
        virtual ~IpodComposer();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        //IpodComposer specific methods
        void addTrack( IpodTrackPtr track );
        void remTrack( IpodTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class IpodYear : public Meta::Year
{
    public:
        IpodYear( const QString &name );
        virtual ~IpodYear();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        //IpodYear specific methods
        void addTrack( IpodTrackPtr track );
        void remTrack( IpodTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

}

#endif


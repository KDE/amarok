/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef MEDIADEVICEMETA_H
#define MEDIADEVICEMETA_H

#include "Debug.h"
#include "Meta.h"
#include "mediadevicecollection_export.h"

#include <QMultiMap>

class MediaDeviceCollection;
class QAction;

namespace Meta
{

class MediaDeviceTrack;
class MediaDeviceAlbum;
class MediaDeviceArtist;
class MediaDeviceGenre;
class MediaDeviceComposer;
class MediaDeviceYear;

typedef KSharedPtr<MediaDeviceTrack> MediaDeviceTrackPtr;
typedef KSharedPtr<MediaDeviceArtist> MediaDeviceArtistPtr;
typedef KSharedPtr<MediaDeviceAlbum> MediaDeviceAlbumPtr;
typedef KSharedPtr<MediaDeviceGenre> MediaDeviceGenrePtr;
typedef KSharedPtr<MediaDeviceComposer> MediaDeviceComposerPtr;
typedef KSharedPtr<MediaDeviceYear> MediaDeviceYearPtr;

class MEDIADEVICECOLLECTION_EXPORT MediaDeviceTrack : public Meta::Track
{

    public:
        MediaDeviceTrack( MediaDeviceCollection *collection );
        virtual ~MediaDeviceTrack();

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
        void setLastPlayed( const uint newTime );

        virtual int playCount() const;
        void setPlayCount( const int newCount );

        virtual QString type() const;
        virtual void prepareToPlay();

        virtual void beginMetaDataUpdate() { DEBUG_BLOCK }
        virtual void endMetaDataUpdate();
        virtual void abortMetaDataUpdate() { DEBUG_BLOCK }
/*
        virtual void subscribe ( Observer *observer );
        virtual void unsubscribe ( Observer *observer );
*/
        virtual bool inCollection() const;
        virtual Amarok::Collection* collection() const;

	virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const;
//    virtual Meta::Capability* asCapabilityInterface( Meta::Capability::Type type );
	virtual Meta::Capability* createCapabilityInterface( Meta::Capability::Type type );
    

        //MediaDeviceTrack specific methods

        // These methods are for Handler
        void setAlbum( MediaDeviceAlbumPtr album );
        void setArtist( MediaDeviceArtistPtr artist );
        void setComposer( MediaDeviceComposerPtr composer );
        void setGenre( MediaDeviceGenrePtr genre );
        void setYear( MediaDeviceYearPtr year );

        void setType( const QString & type );

        void setLength( int length );
    void setPlayableUrl( const KUrl &url) { m_playableUrl = url; }

    private:
        MediaDeviceCollection *m_collection;

        MediaDeviceArtistPtr m_artist;
        MediaDeviceAlbumPtr m_album;
        MediaDeviceGenrePtr m_genre;
        MediaDeviceComposerPtr m_composer;
        MediaDeviceYearPtr m_year;

        // For MediaDeviceTrack-specific use

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
        uint m_lastPlayed;
        int m_rating;
        float m_bpm;
        QString m_displayUrl;
        KUrl m_playableUrl;
};

class MEDIADEVICECOLLECTION_EXPORT MediaDeviceArtist : public Meta::Artist
{
    public:
        MediaDeviceArtist( const QString &name );
        virtual ~MediaDeviceArtist();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();
        virtual AlbumList albums();

        //MediaDeviceArtist specific methods
        void addTrack( MediaDeviceTrackPtr track );
        void remTrack( MediaDeviceTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class MEDIADEVICECOLLECTION_EXPORT MediaDeviceAlbum : public Meta::Album
{
    public:
        MediaDeviceAlbum( MediaDeviceCollection *collection, const QString &name );
        virtual ~MediaDeviceAlbum();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual bool isCompilation() const;
        void setIsCompilation( bool compilation );

        virtual bool hasAlbumArtist() const;
        virtual ArtistPtr albumArtist() const;
        virtual TrackList tracks();

        virtual bool hasImage( int size = 1 ) const;
        virtual QPixmap image( int size = 1 );
        virtual bool canUpdateImage() const;
        virtual void setImage( const QPixmap &pixmap );
        void setImagePath( const QString &path );
        virtual void removeImage();

        virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const;
        virtual Meta::Capability* asCapabilityInterface( Meta::Capability::Type type );

        //MediaDeviceAlbum specific methods

        void addTrack( MediaDeviceTrackPtr track );
        void remTrack( MediaDeviceTrackPtr track );
        void setAlbumArtist( MediaDeviceArtistPtr artist );

    private:
        MediaDeviceCollection *m_collection;

        QString         m_name;
        QString         m_coverPath;
        TrackList       m_tracks;
        bool            m_isCompilation;
        mutable bool    m_hasImage;
        bool            m_hasImageChecked;
        QPixmap         m_image;
        MediaDeviceArtistPtr   m_albumArtist;
};

class MEDIADEVICECOLLECTION_EXPORT MediaDeviceComposer : public Meta::Composer
{
    public:
        MediaDeviceComposer( const QString &name );
        virtual ~MediaDeviceComposer();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        //MediaDeviceComposer specific methods
        void addTrack( MediaDeviceTrackPtr track );
        void remTrack( MediaDeviceTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class MEDIADEVICECOLLECTION_EXPORT MediaDeviceGenre : public Meta::Genre
{
    public:
        MediaDeviceGenre( const QString &name );
        virtual ~MediaDeviceGenre();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        //MediaDeviceGenre specific methods
        void addTrack( MediaDeviceTrackPtr track );
        void remTrack( MediaDeviceTrackPtr track );
    private:
        QString m_name;
        TrackList m_tracks;
};



class MEDIADEVICECOLLECTION_EXPORT MediaDeviceYear : public Meta::Year
{
    public:
        MediaDeviceYear( const QString &name );
        virtual ~MediaDeviceYear();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        //MediaDeviceYear specific methods
        void addTrack( MediaDeviceTrackPtr track );
        void remTrack( MediaDeviceTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

}

#endif


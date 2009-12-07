/****************************************************************************************
 * Copyright (c) 2007-2009 Bart Cerneels <bart.cerneels@kde.org>                        *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef PODCASTMETA_H
#define PODCASTMETA_H

#include "core/support/Amarok.h"
#include "shared/amarok_export.h"
#include "core/meta/Meta.h"
#include "core/playlists/Playlist.h"

#include <KLocale>
#include <KUrl>

#include <QDateTime>
#include <QSharedData>
#include <QString>
#include <QStringList>
#include <QTextStream>


namespace Podcasts
{

class PodcastMetaCommon;
class PodcastEpisode;
class PodcastChannel;

class PodcastArtist;
class PodcastAlbum;
class PodcastComposer;
class PodcastGenre;
class PodcastYear;

// typedef KSharedPtr<PodcastMetaCommon> PodcastMetaCommonPtr;
typedef KSharedPtr<PodcastEpisode> PodcastEpisodePtr;
typedef KSharedPtr<PodcastChannel> PodcastChannelPtr;

// typedef QList<PodcastMetaCommon> PodcastMetaCommonList;
typedef QList<PodcastEpisodePtr> PodcastEpisodeList;
typedef QList<PodcastChannelPtr> PodcastChannelList;

enum PodcastType
{
    NoType = 0,
    ChannelType,
    EpisodeType
};

class AMAROK_CORE_EXPORT PodcastMetaCommon
{
    public:
        PodcastMetaCommon() {}
        virtual ~PodcastMetaCommon() {}

        virtual QString title() const { return m_title;}
        virtual QString description() const { return m_description; }
        virtual QStringList keywords() const { return m_keywords; }
        virtual QString subtitle() const { return m_subtitle; }
        virtual QString summary() const { return m_summary; }
        virtual QString author() const { return m_author; }

        virtual void setTitle( const QString &title ) { m_title = title; }
        virtual void setDescription( const QString &description ) { m_description = description; }
        virtual void setKeywords( const QStringList &keywords ) { m_keywords = keywords; }
        virtual void addKeyword( const QString &keyword ) { m_keywords << keyword; }
        virtual void setSubtitle( const QString &subtitle ) { m_subtitle = subtitle; }
        virtual void setSummary( const QString &summary ) { m_summary = summary; }
        virtual void setAuthor( const QString &author ) { m_author = author; }

        virtual int podcastType() = 0; //{ return NoType; }

    protected:
        QString m_title; //the title
        QString m_description; //a longer description, with HTML markup
        QStringList m_keywords; // TODO: save to DB
        QString m_subtitle; //a short description
        QString m_summary;
        QString m_author; // TODO: save to DB
};

class AMAROK_CORE_EXPORT PodcastEpisode : public PodcastMetaCommon, public Meta::Track
{
    public:
        PodcastEpisode();
        PodcastEpisode( PodcastChannelPtr channel );
        PodcastEpisode( PodcastEpisodePtr episode, PodcastChannelPtr channel );

        virtual ~PodcastEpisode() {}

        //MetaBase methods
        virtual QString name() const { return m_title; }
        virtual QString prettyName() const { return m_title; }

        //Track Methods
        virtual KUrl playableUrl() const { return m_localUrl.isEmpty() ? m_url : m_localUrl; }
        virtual QString prettyUrl() const { return playableUrl().prettyUrl(); }
        virtual QString uidUrl() const { return m_url.url(); }
        virtual bool isPlayable() const { return true; }
        virtual bool isEditable() const { return false; }

        virtual Meta::AlbumPtr album() const { return m_albumPtr; }
        virtual Meta::ArtistPtr artist() const { return m_artistPtr; }
        virtual Meta::ComposerPtr composer() const { return m_composerPtr; }
        virtual Meta::GenrePtr genre() const { return m_genrePtr; }
        virtual Meta::YearPtr year() const { return m_yearPtr; }

        virtual void setTitle( const QString &title ) { m_title = title; }

        virtual qreal bpm() const { return -1.0; }

        virtual QString comment() const { return QString(); }
        virtual void setComment( const QString &newComment ) { Q_UNUSED( newComment ); }
        virtual double score() const { return 0; }
        virtual void setScore( double newScore ) { Q_UNUSED( newScore ); }
        virtual int rating() const { return 0; }
        virtual void setRating( int newRating ) { Q_UNUSED( newRating ); }
        virtual qint64 length() const { return m_duration * 1000; }
        virtual int filesize() const { return m_fileSize; }
        virtual int sampleRate() const { return 0; }
        virtual int bitrate() const { return 0; }
        virtual int trackNumber() const { return m_sequenceNumber; }
        virtual void setTrackNumber( int newTrackNumber ) { Q_UNUSED( newTrackNumber ); }
        virtual int discNumber() const { return 0; }
        virtual void setDiscNumber( int newDiscNumber ) { Q_UNUSED( newDiscNumber ); }
        virtual uint lastPlayed() const { return 0; }
        virtual int playCount() const { return 0; }
        virtual QString mimeType() const { return m_mimeType; }

        virtual QString type() const
        {
            const QString fileName = playableUrl().fileName();
            return Amarok::extension( fileName );
        }

        virtual void finishedPlaying( double playedFraction ) { Q_UNUSED( playedFraction ); }
        virtual void addMatchTo( Collections::QueryMaker* qm ) { Q_UNUSED( qm ); }
        virtual bool inCollection() const { return false; }
        virtual QString cachedLyrics() const { return QString(); }
        virtual void setCachedLyrics( const QString &lyrics ) { Q_UNUSED( lyrics ); }

        //PodcastMetaCommon methods
        int podcastType() { return EpisodeType; }

        //PodcastEpisode methods
        virtual KUrl localUrl() const { return m_localUrl; }
        virtual void setLocalUrl( const KUrl &url ) { m_localUrl = url; }
        virtual QDateTime pubDate() const { return m_pubDate; }
        virtual int duration() const { return m_duration; }
        virtual QString guid() const { return m_guid; }
        virtual bool isNew() const { return m_isNew; }

        virtual void setFilesize( int fileSize ) { m_fileSize = fileSize; }
        virtual void setMimeType( const QString &mimeType ) { m_mimeType = mimeType; }
        virtual void setUidUrl( const KUrl &url ) { m_url = url; }
        virtual void setPubDate( const QDateTime &pubDate ) { m_pubDate = pubDate; }
        virtual void setDuration( int duration ) { m_duration = duration; }
        virtual void setGuid( const QString &guid ) { m_guid = guid; }
        virtual void setNew( bool isNew ) { m_isNew = isNew; }

        virtual int sequenceNumber() const { return m_sequenceNumber; }
        virtual void setSequenceNumber( int sequenceNumber ) { m_sequenceNumber = sequenceNumber; }

        virtual PodcastChannelPtr channel() const { return m_channel; }
        virtual void setChannel( const PodcastChannelPtr channel ) { m_channel = channel; }

    protected:
        PodcastChannelPtr m_channel;

        QString m_guid; //the GUID from the podcast feed
        KUrl m_url; //remote url of the file
        KUrl m_localUrl; //the localUrl, only valid if downloaded
        QString m_mimeType; //the mimetype of the enclosure
        QDateTime m_pubDate; //the pubDate from the feed
        int m_duration; //the playlength in seconds
        int m_fileSize; //the size tag from the enclosure
        int m_sequenceNumber; //number of the episode
        bool m_isNew; //listened to or not?

        //data members
        Meta::AlbumPtr m_albumPtr;
        Meta::ArtistPtr m_artistPtr;
        Meta::ComposerPtr m_composerPtr;
        Meta::GenrePtr m_genrePtr;
        Meta::YearPtr m_yearPtr;
};

class AMAROK_CORE_EXPORT PodcastChannel : public PodcastMetaCommon, public Playlists::Playlist
{
    public:

        enum FetchType
        {
            DownloadWhenAvailable = 0,
            StreamOrDownloadOnDemand
        };

        PodcastChannel()
            : PodcastMetaCommon()
            , Playlist()
            , m_image()
            , m_subscribeDate()
            , m_copyright()
            , m_autoScan( false )
            , m_fetchType( DownloadWhenAvailable )
            , m_purge( false )
            , m_purgeCount( 0 )
        { }

        PodcastChannel( Podcasts::PodcastChannelPtr channel );
        virtual ~PodcastChannel() {}

        //Playlist virtual methods
        virtual KUrl uidUrl() const { return m_url; }
        virtual QString name() const { return title(); }
        virtual QString prettyName() const { return title(); }

        virtual Meta::TrackList tracks();
        virtual void addTrack( Meta::TrackPtr track, int position = -1 );

        //PodcastMetaCommon methods
        int podcastType() { return ChannelType; }
        //override this since it's ambigous in PodcastMetaCommon and Playlist
        virtual QString description() const { return m_description; }

        //PodcastChannel specific methods
        virtual KUrl url() const { return m_url; }
        virtual KUrl webLink() const { return m_webLink; }
        virtual bool hasImage() const { return !m_image.isNull(); }
        virtual KUrl imageUrl() const { return m_imageUrl; }
        virtual QPixmap image() const { return m_image; }
        virtual QString copyright() { return m_copyright; }
        virtual QStringList labels() const { return m_labels; }
        virtual QDate subscribeDate() const { return m_subscribeDate; }

        virtual void setUrl( const KUrl &url ) { m_url = url; }
        virtual void setWebLink( const KUrl &link ) { m_webLink = link; }
        virtual void setImage( const QPixmap &image ) { m_image = image; }
        virtual void setImageUrl( const KUrl &imageUrl ) { m_imageUrl = imageUrl; }
        virtual void setCopyright( const QString &copyright ) { m_copyright = copyright; }
        virtual void setLabels( const QStringList &labels ) { m_labels = labels; }
        virtual void addLabel( const QString &label ) { m_labels << label; }
        virtual void setSubscribeDate( const QDate &date ) { m_subscribeDate = date; }

        virtual Podcasts::PodcastEpisodePtr addEpisode( PodcastEpisodePtr episode );
        virtual PodcastEpisodeList episodes() { return m_episodes; }

        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const { Q_UNUSED( type ); return false; }

        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) { Q_UNUSED( type ); return static_cast<Capabilities::Capability *>( 0 ); }

        bool load( QTextStream &stream ) { Q_UNUSED( stream ); return false; }

        //Settings
        KUrl saveLocation() const { return m_directory; }
        bool autoScan() { return m_autoScan; }
        FetchType fetchType() { return m_fetchType; }
        bool hasPurge() { return m_purge; }
        int purgeCount() { return m_purgeCount; }

        void setSaveLocation( const KUrl &url ) { m_directory = url; }
        void setAutoScan( bool autoScan ) { m_autoScan = autoScan; }
        void setFetchType( FetchType fetchType ) { m_fetchType = fetchType; }
        void setPurge( bool purge ) { m_purge = purge; }
        void setPurgeCount( int purgeCount ) { m_purgeCount = purgeCount; }

    protected:
        KUrl m_url;
        KUrl m_webLink;
        QPixmap m_image;
        KUrl m_imageUrl;
        QStringList m_labels;
        QDate m_subscribeDate;
        QString m_copyright;
        KUrl m_directory; //the local directory to save the files in.
        bool m_autoScan; //should this channel be checked automatically?
        PodcastChannel::FetchType m_fetchType; //'download when available' or 'stream or download on demand'
        bool m_purge; //remove old episodes?
        int m_purgeCount; //how many episodes do we keep on disk?

        PodcastEpisodeList m_episodes;
};

// internal helper classes

class AMAROK_CORE_EXPORT PodcastArtist : public Meta::Artist
{
public:
    PodcastArtist( PodcastEpisode *episode )
        : Meta::Artist()
        , episode( episode )
    {}

    Meta::TrackList tracks()
    {
        return Meta::TrackList();
    }

    Meta::AlbumList albums()
    {
        return Meta::AlbumList();
    }

    QString name() const
    {
        QString author;
        if( episode && episode->channel() )
            author = episode->channel()->author();

        return author;
    }

    QString prettyName() const
    {
        return name();
    }

    bool operator==( const Meta::Artist &other ) const
    {
        return name() == other.name();
    }

    PodcastEpisode const *episode;
};

class AMAROK_CORE_EXPORT PodcastAlbum : public Meta::Album
{
public:
    PodcastAlbum( PodcastEpisode *episode )
        : Meta::Album()
        , episode( episode )
    {}

    bool isCompilation() const
    {
        return false;
    }

    bool hasAlbumArtist() const
    {
        return false;
    }

    Meta::ArtistPtr albumArtist() const
    {
        return Meta::ArtistPtr();
    }

    Meta::TrackList tracks()
    {
        return Meta::TrackList();
    }

    QString name() const
    {
        if( episode != 0 )
        {
            const QString albumName = episode->channel()->title();
            return albumName;
        }
        else
            return QString();
    }

    QString prettyName() const
    {
        return name();
    }

    QPixmap image( int size )
    {
        QPixmap image = episode->channel()->image();
        return image.scaledToHeight( size );
    }

    bool operator==( const Meta::Album &other ) const
    {
        return name() == other.name();
    }

    PodcastEpisode const *episode;
};

class AMAROK_CORE_EXPORT PodcastGenre : public Meta::Genre
{
public:
    PodcastGenre( PodcastEpisode *episode )
        : Meta::Genre()
        , episode( episode )
    {}

    Meta::TrackList tracks()
    {
        return Meta::TrackList();
    }

    QString name() const
    {
        const QString genreName = i18n( "Podcast" );
        return genreName;
    }

    QString prettyName() const
    {
        return name();
    }

    bool operator==( const Meta::Genre &other ) const
    {
        return name() == other.name();
    }

    PodcastEpisode const *episode;
};

class AMAROK_CORE_EXPORT PodcastComposer : public Meta::Composer
{
public:
    PodcastComposer( PodcastEpisode *episode )
        : Meta::Composer()
        , episode( episode )
    {}

    Meta::TrackList tracks()
    {
        return Meta::TrackList();
    }

    QString name() const
    {
        if( episode != 0 )
        {
            const QString composer = episode->channel()->author();
            return composer;
        }
        else
            return QString();

     }

    QString prettyName() const
    {
        return name();
    }

    bool operator==( const Meta::Composer &other ) const
    {
        return name() == other.name();
    }

    PodcastEpisode const *episode;
};

class AMAROK_CORE_EXPORT PodcastYear : public Meta::Year
{
public:
    PodcastYear( PodcastEpisode *episode )
        : Meta::Year()
        , episode( episode )
    {}

    Meta::TrackList tracks()
    {
        return Meta::TrackList();
    }

    QString name() const
    {
        if( episode != 0 )
        {
            const QString year = episode->pubDate().toString( "yyyy" );
            return year;
        }
        else
            return QString();
    }

    QString prettyName() const
    {
        return name();
    }

    bool operator==( const Meta::Year &other ) const
    {
        return name() == other.name();
    }

    PodcastEpisode const *episode;
};

} //namespace Podcasts

Q_DECLARE_METATYPE( Podcasts::PodcastMetaCommon* )
Q_DECLARE_METATYPE( Podcasts::PodcastEpisodePtr )
Q_DECLARE_METATYPE( Podcasts::PodcastEpisodeList )
Q_DECLARE_METATYPE( Podcasts::PodcastChannelPtr )
Q_DECLARE_METATYPE( Podcasts::PodcastChannelList )

#endif

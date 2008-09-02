/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@kde.org>

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

#ifndef PODCASTMETA_H
#define PODCASTMETA_H

#include "Debug.h"
#include "Meta.h"
#include "Playlist.h"

#include <kurl.h>
#include <KLocale>

#include <QSharedData>
#include <QString>
#include <QStringList>
#include <QTextStream>

class QDate;

namespace Meta
{

class PodcastMetaCommon;
class PodcastEpisode;
class PodcastChannel;

// typedef KSharedPtr<PodcastMetaCommon> PodcastMetaCommonPtr;
typedef KSharedPtr<PodcastEpisode> PodcastEpisodePtr;
typedef KSharedPtr<PodcastChannel> PodcastChannelPtr;

// typedef QList<PodcastMetaCommon> PodcastMetaCommonList;
typedef QList<PodcastEpisodePtr> PodcastEpisodeList;
typedef QList<PodcastChannelPtr> PodcastChannelList;


enum Type
{
    NoType = 0,
    ChannelType,
    EpisodeType
};



class PodcastMetaCommon
{
    public:

        PodcastMetaCommon() {};
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
        QString m_description; //a longer description, possibly with HTML markup
        QStringList m_keywords;
        QString m_subtitle; //a short description
        QString m_summary;
        QString m_author;
};

class PodcastEpisode : public PodcastMetaCommon, public Track
{
    public:
        PodcastEpisode()
            : PodcastMetaCommon()
            , Track()
        {}

        PodcastEpisode( PodcastChannelPtr channel )
            : PodcastMetaCommon()
            , Track()
            , m_channel( channel ) {}

        virtual ~PodcastEpisode() {}

        //MetaBase methods
        virtual QString name() const { return m_title; }
        virtual QString prettyName() const { return m_title; }

        //Track Methods
        virtual KUrl playableUrl() const { return m_localUrl.isEmpty() ? m_url : m_localUrl; }
        virtual QString prettyUrl() const { return m_url.prettyUrl(); }
        virtual QString uidUrl() const { return m_url.url(); }
        virtual bool isPlayable() const { return true; }
        virtual bool isEditable() const { return false; }

        virtual AlbumPtr album() const { return AlbumPtr(); }
        virtual void setAlbum( const QString &newAlbum ) { Q_UNUSED( newAlbum ); }
        virtual ArtistPtr artist() const { return ArtistPtr(); }
        virtual void setArtist( const QString &newArtist ) { Q_UNUSED( newArtist ); }
        virtual ComposerPtr composer() const { return ComposerPtr(); }
        virtual void setComposer( const QString &newComposer ) { Q_UNUSED( newComposer ); }
        virtual GenrePtr genre() const { return GenrePtr(); }
        virtual void setGenre( const QString &newGenre ) { Q_UNUSED( newGenre ); }
        virtual YearPtr year() const { return YearPtr(); }
        virtual void setYear( const QString &newYear ) { Q_UNUSED( newYear ); }

        virtual void setTitle( const QString &title ) { m_title = title; }

        virtual QString comment() const { return QString(); }
        virtual void setComment( const QString &newComment ) { Q_UNUSED( newComment ); }
        virtual double score() const { return 0; }
        virtual void setScore( double newScore ) { Q_UNUSED( newScore ); }
        virtual int rating() const { return 0; }
        virtual void setRating( int newRating ) { Q_UNUSED( newRating ); }
        virtual int length() const { return m_duration; }
        virtual int filesize() const { return m_fileSize; }
        virtual int sampleRate() const { return 0; }
        virtual int bitrate() const { return 0; }
        virtual int trackNumber() const { return m_sequenceNumber; }
        virtual void setTrackNumber( int newTrackNumber ) { Q_UNUSED( newTrackNumber ); }
        virtual int discNumber() const { return 0; }
        virtual void setDiscNumber( int newDiscNumber ) { Q_UNUSED( newDiscNumber ); }
        virtual uint lastPlayed() const { return 0; }
        virtual int playCount() const { return 0; }

        virtual QString type() const { return i18n( "Podcast" ); }

        virtual void beginMetaDataUpdate() {}
        virtual void endMetaDataUpdate() {}
        virtual void abortMetaDataUpdate() {}
        virtual void finishedPlaying( double playedFraction ) { Q_UNUSED( playedFraction ); }
        virtual void addMatchTo( QueryMaker* qm ) { Q_UNUSED( qm ); }
        virtual bool inCollection() const { return false; }
        virtual QString cachedLyrics() const { return QString(); }
        virtual void setCachedLyrics( const QString &lyrics ) { Q_UNUSED( lyrics ); }
        virtual void notifyObservers() const {}

        //PodcastMetaCommon methods
        int podcastType() { return EpisodeType; }

        //PodcastEpisode methods
        KUrl localUrl() const { return m_localUrl; }
        void setLocalUrl( const KUrl &url ) { m_localUrl = url; }
        QString pubDate() const { return m_pubDate; }
        int duration() const { return m_duration; }
        QString guid() const { return m_guid; }

        void setUidUrl( const KUrl &url ) { m_url = url; }
        void setPubDate( const QString &pubDate ) { m_pubDate = pubDate; }
        void setDuration( int duration ) { m_duration = duration; }
        void setGuid( const QString &guid ) { m_guid = guid; }

        int sequenceNumber() const { return m_sequenceNumber; }
        void setSequenceNumber( int sequenceNumber ) { m_sequenceNumber = sequenceNumber; }

        PodcastChannelPtr channel() { return m_channel; }
        void setChannel( const PodcastChannelPtr channel ) { m_channel = channel; }

    protected:
        PodcastChannelPtr m_channel;

        QString m_guid; //the GUID from the podcast feed
        KUrl m_url; //remote url of the file
        KUrl m_localUrl; //the localUrl, only valid if downloaded
        QString m_mimeType; //the mimetype of the enclosure
        //TODO: convert to QDateTime from a RFC822 format
        QString m_pubDate; //the pubDate from the feed
        int m_duration; //the playlength in seconds
        int m_fileSize; //the size tag from the enclosure
        int m_sequenceNumber; //number of the episode
        bool m_isNew; //listened to or not?

};

class PodcastChannel : public PodcastMetaCommon, public Playlist
{
    public:

        enum FetchType
        {
            DownloadWhenAvailable = 0,
            StreamOrDownloadOnDemand
        };

        PodcastChannel()
            : PodcastMetaCommon()
            , Playlist() { DEBUG_BLOCK }

        virtual ~PodcastChannel() {}

        //Playlist virtual methods
        virtual QString name() const { return title(); }
        virtual QString prettyName() const { return title(); }

        virtual TrackList tracks() { return m_tracks; }

        virtual KUrl retrievableUrl() { return KUrl(); }

        //PodcastMetaCommon methods
        int podcastType() { return ChannelType; }

        //PodcastChannel specific methods
        KUrl url() const { return m_url; }
        KUrl webLink() const { return m_webLink; }
        QPixmap image() const { return m_image; }
        QString copyright() { return m_copyright; }
        QStringList labels() const { return m_labels; }
        QDate subscribeDate() const { return m_subscribeDate; }

        void setUrl( const KUrl &url ) { m_url = url; }
        void setWebLink( const KUrl &link ) { m_webLink = link; }
        void setImage( const QPixmap &image ) { m_image = image; }
        void setCopyright( const QString &copyright ) { m_copyright = copyright; }
        void setLabels( const QStringList &labels ) { m_labels = labels; }
        void addLabel( const QString &label ) { m_labels << label; }
        void setSubscribeDate( const QDate &date ) { m_subscribeDate = date; }

        void addEpisode( PodcastEpisodePtr episode ) { m_episodes << episode; }
        PodcastEpisodeList episodes() { return m_episodes; }

        bool hasCapabilityInterface( Meta::Capability::Type type ) const { Q_UNUSED( type ); return false; }

        Meta::Capability* asCapabilityInterface( Meta::Capability::Type type ) { Q_UNUSED( type ); return static_cast<Meta::Capability *>( 0 ); }

        bool load( QTextStream &stream ) { Q_UNUSED( stream ); return false; }

        //Settings
        bool autoScan() { return m_autoScan; }
        FetchType fetchType() { return m_fetchType; }
        bool hasPurge() { return m_purge; }
        int purgeCount() { return m_purgeCount; }

        void setAutoscan( bool autoScan ) { m_autoScan = autoScan; }
        void setFetchType( FetchType fetchType ) { m_fetchType = fetchType; }
        void setPurge( bool purge ) { m_purge = purge; }
        void setPurgeCount( int purgeCount ) { m_purgeCount = purgeCount; }

    protected:
        KUrl m_url;
        KUrl m_webLink;
        QPixmap m_image;
        QStringList m_labels;
        QDate m_subscribeDate;
        QString m_copyright;
        KUrl m_directory; //the local directory to save the files in.
        bool m_autoScan; //should this channel be checked automatically?
        PodcastChannel::FetchType m_fetchType; //'download when available' or 'stream or download on demand'
        bool m_purge; //remove old episodes?
        int m_purgeCount; //how many episodes do we keep on disk?

        PodcastEpisodeList m_episodes;
        TrackList m_tracks;
};

}

// Q_DECLARE_METATYPE( Meta::PodcastMetaCommonPtr )
// Q_DECLARE_METATYPE( Meta::PodcastMetaCommonList )
Q_DECLARE_METATYPE( Meta::PodcastEpisodePtr )
Q_DECLARE_METATYPE( Meta::PodcastEpisodeList )
Q_DECLARE_METATYPE( Meta::PodcastChannelPtr )
Q_DECLARE_METATYPE( Meta::PodcastChannelList )

#endif

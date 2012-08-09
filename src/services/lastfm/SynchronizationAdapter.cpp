/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "SynchronizationAdapter.h"

#include "MetaValues.h"
#include "core/support/Debug.h"

#include <KLocalizedString>

#include <QNetworkReply>

#include <Library.h>
#include <XmlQuery.h>

class AdHocTrack : public StatSyncing::Track
{
    public:
        AdHocTrack( QString artist, QString album, QString name, int playCount )
            : m_artist( artist ), m_album( album ), m_name( name ), m_playCount( playCount ) {}

        virtual QString name() const { return m_name; }
        virtual QString album() const { return m_album; }
        virtual QString artist() const { return m_artist; }
        virtual int rating() const { return 0; }
        virtual void setRating( int rating ) {}
        virtual QDateTime firstPlayed() const { return QDateTime(); }
        virtual void setFirstPlayed( const QDateTime &firstPlayed ) {}
        virtual QDateTime lastPlayed() const { return QDateTime(); }
        virtual void setLastPlayed( const QDateTime &lastPlayed ) {}
        virtual int playCount() const { return m_playCount; }
        virtual void setPlayCount( int playCount ) {}
        virtual QSet<QString> labels() const { return m_labels; }
        virtual void setLabels( const QSet<QString> &labels ) {}
        virtual void commit() {}

        /**
         * Set tags from Last.fm
         */
        void setLastFmTags( const QSet<QString> &tags ) { m_labels = tags; }

    private:
        QString m_artist;
        QString m_album;
        QString m_name;
        int m_playCount;
        QSet<QString> m_labels;
};

/**
 * Helper class that releases passed QSemaphore upon deletion. Similar to QMutexLocker.
 */
class SemaphoreReleaser
{
    public:
        SemaphoreReleaser( QSemaphore *semaphore ) : m_semaphore( semaphore ) {}
        ~SemaphoreReleaser() { if( m_semaphore ) m_semaphore->release(); }

        /**
         * Tell SemaphoreReleaser not to release the semaphore upon deletion.
         */
        void dontRelease() { m_semaphore = 0; }

    private:
        QSemaphore *m_semaphore;
};

SynchronizationAdapter::SynchronizationAdapter( const QString &user )
    : m_user( user )
{
    connect( this, SIGNAL(startArtistSearch(int)),
             SLOT(slotStartArtistSearch(int)), Qt::QueuedConnection );
    connect( this, SIGNAL(startTrackSearch(QString,int)),
             SLOT(slotStartTrackSearch(QString,int)), Qt::QueuedConnection );
    connect( this, SIGNAL(startTagSearch(QString,QString)),
             SLOT(slotStartTagSearch(QString,QString)), Qt::QueuedConnection );
}

SynchronizationAdapter::~SynchronizationAdapter()
{
}

QString
SynchronizationAdapter::id() const
{
    return "lastfm";
}

QString
SynchronizationAdapter::prettyName() const
{
    return i18n( "Last.fm" );
}

KIcon
SynchronizationAdapter::icon() const
{
    return KIcon( "view-services-lastfm-amarok" );
}

qint64
SynchronizationAdapter::reliableTrackMetaData() const
{
    return Meta::valArtist | Meta::valAlbum | Meta::valTitle;
}

qint64
SynchronizationAdapter::writableTrackStatsData() const
{
    return Meta::valLabel; // TODO
}

StatSyncing::Provider::Preference
SynchronizationAdapter::defaultPreference()
{
    return StatSyncing::Provider::Never; // don't overload Last.fm servers
}

QSet<QString>
SynchronizationAdapter::artists()
{
    DEBUG_BLOCK
    Q_ASSERT( m_semaphore.available() == 0 );
    emit startArtistSearch( 1 ); // Last.fm indexes from 1

    m_semaphore.acquire();
    QSet<QString> ret = m_artists;
    m_artists.clear(); // save memory
    debug() << __PRETTY_FUNCTION__ << ret.count() << "artists total";
    return ret;
}

StatSyncing::TrackList
SynchronizationAdapter::artistTracks( const QString &artistName )
{
    DEBUG_BLOCK
    Q_ASSERT( m_semaphore.available() == 0 );
    emit startTrackSearch( artistName, 1 ); // Last.fm indexes from 1

    m_semaphore.acquire();
    debug() << __PRETTY_FUNCTION__ << m_tracks.count() << "tracks from" << artistName;

    // fetch tags
    QMutableListIterator<StatSyncing::TrackPtr> it( m_tagQueue );
    while( it.hasNext() )
    {
        StatSyncing::TrackPtr track = it.next();
        emit startTagSearch( track->artist(), track->name() );
        m_semaphore.acquire();
        it.remove();
    }

    StatSyncing::TrackList ret = m_tracks;
    m_tracks.clear(); // save memory
    m_tagQueue.clear(); // paranoia
    return ret;
}

void
SynchronizationAdapter::slotStartArtistSearch( int page )
{
    m_currentRequest = lastfm::Library::getArtists( m_user, 200, page );
    connect( m_currentRequest.data(), SIGNAL(finished()), SLOT(slotArtistsReceived()) );
}

void
SynchronizationAdapter::slotStartTrackSearch( QString artistName, int page )
{
    lastfm::Artist artist( artistName );
    m_currentRequest = lastfm::Library::getTracks( m_user, artist, 200, page );
    connect( m_currentRequest.data(), SIGNAL(finished()), SLOT(slotTracksReceived()) );
}

void
SynchronizationAdapter::slotStartTagSearch( QString artistName, QString trackName )
{
    lastfm::MutableTrack track;
    track.setArtist( artistName );
    track.setTitle( trackName );
    m_currentRequest = track.getTags();
    connect( m_currentRequest.data(), SIGNAL(finished()), SLOT(slotTagsReceived()) );
}

void
SynchronizationAdapter::slotArtistsReceived()
{
    SemaphoreReleaser releaser( &m_semaphore );
    Q_ASSERT( m_currentRequest );

    lastfm::XmlQuery lfm;
    if( !lfm.parse( m_currentRequest.data()->readAll() ) )
    {
        warning() << __PRETTY_FUNCTION__ << "Error parsing Last.fm reply:" << lfm.parseError().message();
        return;
    }
    lastfm::XmlQuery artists = lfm[ "artists" ];
    bool ok = false;
    int page = artists.attribute( "page" ).toInt( &ok );
    if( !ok )
    {
        warning() << __PRETTY_FUNCTION__ << "cannot read page number";
        return;
    }
    int totalPages = artists.attribute( "totalPages" ).toInt( &ok );
    if( !ok )
    {
        warning() << __PRETTY_FUNCTION__ << "cannot read total number or pages";
        return;
    }
    debug() << __PRETTY_FUNCTION__ << "page" << page << "of" << totalPages;

    // following is based on lastfm::Artist::list():
    foreach( lastfm::XmlQuery xq, lfm.children( "artist" ) )
    {
        lastfm::Artist artist( xq );
        m_artists.insert( artist.name().toLower() );
    }
    m_currentRequest.data()->deleteLater();
    m_currentRequest = (QNetworkReply *) 0;

    // Last.fm indexes from 1!
    if( page < totalPages )
    {
        releaser.dontRelease(); // don't release the semaphore yet
        emit startArtistSearch( page + 1 );
    }
}

void
SynchronizationAdapter::slotTracksReceived()
{
    SemaphoreReleaser releaser( &m_semaphore );
    Q_ASSERT( m_currentRequest );

    lastfm::XmlQuery lfm;
    if( !lfm.parse( m_currentRequest.data()->readAll() ) )
    {
        warning() << __PRETTY_FUNCTION__ << "Error parsing Last.fm reply:" << lfm.parseError().message();
        return;
    }
    lastfm::XmlQuery tracks = lfm[ "tracks" ];
    bool ok = false;
    int page = tracks.attribute( "page" ).toInt( &ok );
    if( !ok )
    {
        warning() << __PRETTY_FUNCTION__ << "cannot read page number";
        return;
    }
    int totalPages = tracks.attribute( "totalPages" ).toInt( &ok );
    if( !ok )
    {
        warning() << __PRETTY_FUNCTION__ << "cannot read total number or pages";
        return;
    }
    debug() << "page" << page << "of" << totalPages;
    QString searchedArtist = tracks.attribute( "artist" );
    if( searchedArtist.isEmpty() )
    {
        warning() << __PRETTY_FUNCTION__ << "searchedArtist in Last.fm reply is empty";
        return;
    }

    // following is based on lastfm::Track::list():
    foreach( lastfm::XmlQuery xq, lfm.children( "track" ) )
    {
        QString name = xq[ "name" ].text();
        int playCount = xq[ "playcount" ].text().toInt();
        int tagCount = xq[ "tagcount" ].text().toInt();
        QString artist = xq[ "artist" ][ "name" ].text();
        QString album = xq[ "album" ][ "name" ].text();

        StatSyncing::TrackPtr track( new AdHocTrack( artist, album, name, playCount ) );
        m_tracks.append( track );
        if( tagCount > 0 )
            m_tagQueue.append( track );
    }
    m_currentRequest.data()->deleteLater();
    m_currentRequest = (QNetworkReply *) 0;

    // Last.fm indexes from 1!
    if( page < totalPages )
    {
        releaser.dontRelease(); // don't release the semaphore yet
        emit startTrackSearch( searchedArtist, page + 1 );
    }
}

void
SynchronizationAdapter::slotTagsReceived()
{
    SemaphoreReleaser releaser( &m_semaphore );
    Q_ASSERT( m_currentRequest );

    lastfm::XmlQuery lfm;
    if( !lfm.parse( m_currentRequest.data()->readAll() ) )
    {
        warning() << __PRETTY_FUNCTION__ << "Error parsing Last.fm reply:" << lfm.parseError().message();
        return;
    }
    QSet<QString> tags;
    foreach( lastfm::XmlQuery xq, lfm.children( "tag" ) )
    {
        tags.insert( xq[ "name" ].text() );
    }
    Q_ASSERT( !m_tagQueue.isEmpty() );
    AdHocTrack *track = dynamic_cast<AdHocTrack *>( m_tagQueue.first().data() );
    Q_ASSERT( track );
    debug() << "setting tags" << tags << "for" << track->artist() << "-" << track->name();
    track->setLastFmTags( tags );

    m_currentRequest.data()->deleteLater();
    m_currentRequest = (QNetworkReply *) 0;
}

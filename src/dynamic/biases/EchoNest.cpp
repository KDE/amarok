/****************************************************************************************
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
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

#include "EchoNest.h"

#include "Collection.h"
#include "CollectionManager.h"
#include "Debug.h"
#include "EngineController.h"
#include "Meta.h"
#include "QueryMaker.h"
#include "playlist/PlaylistModelStack.h"

#include <kio/job.h>
#include <KComboBox>

#include <QDomDocument>
#include <QVBoxLayout>
#include <QFrame>
#include <QLabel>

// CUSTOM BIAS FACTORY

Dynamic::EchoNestBiasFactory::EchoNestBiasFactory()
: CustomBiasEntryFactory()
{

}

Dynamic::EchoNestBiasFactory::~EchoNestBiasFactory()
{

}

QString
Dynamic::EchoNestBiasFactory::name() const
{

    return i18n( "Echo Nest Similar Artists" );
}

QString
Dynamic::EchoNestBiasFactory::pluginName() const
{
    return "echonest_similarartists";
}



Dynamic::CustomBiasEntry*
Dynamic::EchoNestBiasFactory::newCustomBiasEntry()
{
    debug() << "CREATING ECHONEST BIAS";
    return new EchoNestBias();
}

Dynamic::CustomBiasEntry*
Dynamic::EchoNestBiasFactory::newCustomBiasEntry( QDomElement e )
{
    // we don't save anything, so just load a fresh one
    Q_UNUSED( e )
    debug() << "CREATING ECHONEST BIAS 2";
    return new EchoNestBias();
}


/// class SimilarArtistsBias

Dynamic::EchoNestBias::EchoNestBias()
    : Dynamic::CustomBiasEntry()
    , EngineObserver( The::engineController() )
    , m_artistSuggestedQuery( 0 )
    , m_qm( 0 )
    , m_currentOnly( true )
{
    DEBUG_BLOCK
    engineNewTrackPlaying(); // kick it into gear if a track is already playnig. if not, it's harmless
}

Dynamic::EchoNestBias::~EchoNestBias()
{
    delete m_qm;
}


QString
Dynamic::EchoNestBias::pluginName() const
{
    return "echonest_similarartists";
}

QWidget*
Dynamic::EchoNestBias::configWidget( QWidget* parent )
{
    DEBUG_BLOCK
    QFrame * frame = new QFrame( parent );
    QGridLayout* layout = new QGridLayout( frame );

    layout->addWidget( new QLabel( i18n( "Match:" ), frame ), 0, 0 );

    m_fieldSelection = new KComboBox( parent );
    m_fieldSelection->addItem( i18n( "Current Track" ), "current" );
    m_fieldSelection->addItem( i18n( "Playlist" ), "playlist" );

    connect( m_fieldSelection, SIGNAL( currentIndexChanged(int) ), this, SLOT( selectionChanged( int ) ) );

    layout->addWidget( m_fieldSelection, 0, 1, Qt::AlignLeft );

    QLabel * label = new QLabel( i18n( "Recommendations by Echo Nest." ), parent );
    label->setWordWrap( true );
    label->setAlignment( Qt::AlignCenter );
    layout->addWidget( label, 1, 0, 1, 2, 0 );

    return frame;
}

void
Dynamic::EchoNestBias::engineNewTrackPlaying()
{
    DEBUG_BLOCK
    Meta::TrackPtr track = The::engineController()->currentTrack();

    if( track && track->artist() && !track->artist()->name().isEmpty() )
    {
        m_currentArtist = track->artist()->name(); // save for sure
        // if already saved, don't re-fetch
        if( !m_savedArtists.contains( track->artist()->name() ) || !m_currentOnly )
        {
            if( m_currentOnly )
            {
                QMap< QString, QString > params;

                params[ "query" ] = m_currentArtist;
                params[ "exact" ] = 'Y';
                params[ "sounds_like" ] = 'N'; // mutually exclusive with exact

                KIO::StoredTransferJob* job = KIO::storedGet( createUrl( "search_artists", params ), KIO::NoReload, KIO::HideProgressInfo );
                m_artistNameQueries[ job ] = m_currentArtist;

                connect( job, SIGNAL( result( KJob* ) ), this, SLOT( artistNameQueryDone( KJob* ) ) );
            } else
            { // mode is set to whole playlist, so check if any tracks in the playlist aren't saved as Ids yet and query those
                QList< Meta::TrackPtr > playlist;
                m_currentPlaylist.clear(); // for searching in later
                for( int i = 0; i < The::playlist()->rowCount(); i++ )
                {
                    Meta::TrackPtr t = The::playlist()->trackAt( i );
                    playlist << t;
                    if( !m_currentPlaylist.contains( t->artist()->name() ) )
                        m_currentPlaylist << t->artist()->name();
                }
                // first get any new tracks
                foreach( Meta::TrackPtr track, playlist )
                {
                    if( !m_artistIds.contains( track->artist()->name() ) ) // don't have it yet
                    {
                        debug() << "searching for artistL" << track->artist()->name();
                        QMap< QString, QString > params;

                        params[ "query" ] =  track->artist()->name();
                        params[ "exact" ] = 'Y';
                        params[ "sounds_like" ] = 'N'; // mutually exclusive with exact

                        KIO::StoredTransferJob* job = KIO::storedGet( createUrl( "search_artists", params ), KIO::NoReload, KIO::HideProgressInfo );
                        m_artistNameQueries[ job ] = track->artist()->name();

                        connect( job, SIGNAL( result( KJob* ) ), this, SLOT( artistNameQueryDone( KJob* ) ) );

                        m_artistIds[ track->artist()->name() ] = "-1"; // mark as not being searched for
                    }
                }
                // also go through it to remove any tracks that we don't have
                foreach( const QString &name, m_artistIds.keys() )
                {
                    if( !m_currentPlaylist.contains( name ) )
                        m_artistIds.remove( name );
                }
                debug() << "current playlist:" << m_currentPlaylist;
                // save list of artists
            }
        }
    }
}

void
Dynamic::EchoNestBias::update()
{
    DEBUG_BLOCK
    if( !m_needsUpdating )
        return;

    m_qm->run();
}

void
Dynamic::EchoNestBias::artistNameQueryDone( KJob* job )
{
    DEBUG_BLOCK
    KIO::StoredTransferJob* stjob = static_cast< KIO::StoredTransferJob* >( job );

    if( !m_artistNameQueries.contains( stjob ) )
    {
        debug() << "job was deleted before the slot was called..wtf?";
        return;
    }

    QStringList toQuery;

    QDomDocument doc;
    if( ! doc.setContent( stjob->data() ) )
    {
        debug() << "Got invalid XML from EchoNest on solo artist name query!";
        return;
    }

    QDomNodeList artists = doc.elementsByTagName( "artist" );
    if( artists.isEmpty() )
    {
        debug() << "got no artist for given name :-/";
        return;
    } else
    {
        QDomNode artist = artists.at( 0 );
        QString id = artist.firstChildElement( "id" ).text();
        debug() << "got element ID:" << id;

        if( id.isEmpty() )
        {
            debug() << "Got empty ID though :(";
            return;
        }

        m_artistIds[ m_artistNameQueries[ stjob ] ] = id;

        if( ! m_currentOnly )
        {
            // check our map, see if there are any we are still waiting for. if not, do the query
            foreach( const QString &result, m_artistIds )
            {
                if( result == "-1" ) // still waiting
                {
                    //debug() << "NOT DONE YET WITH QUERY!";
                    //debug() << m_artistIds;
                    job->deleteLater();
                    return;
                }
            }
            const QStringList keys = m_artistIds.keys();
            foreach( const QString &key, keys )
                toQuery << key;
            // ok we're not, update our list and do it!
        } else
            toQuery << m_artistNameQueries[ stjob ];

    }


    // now do our second query
    QMultiMap< QString, QString > params;
    foreach( const QString &name, toQuery )
    {
        params.insert("id", m_artistIds[ name ] );
    }
    params.insert( "rows", "30" );
    m_artistSuggestedQuery = KIO::storedGet( createUrl( "get_similar", params ), KIO::NoReload, KIO::HideProgressInfo );
    connect( m_artistSuggestedQuery, SIGNAL( result( KJob* ) ), this, SLOT( artistSuggestedQueryDone( KJob* ) ) );
    job->deleteLater();
}

void
Dynamic::EchoNestBias::artistSuggestedQueryDone( KJob* job ) // slot
{
    DEBUG_BLOCK

    if( job != m_artistSuggestedQuery )
    {
        debug() << "job was deleted from under us...wtf! blame the gerbils.";
        return;
    }

    QMutexLocker locker( &m_mutex );

    QDomDocument doc;
    if( !doc.setContent( m_artistSuggestedQuery->data() ) )
    {
        debug() << "got invalid XML from EchoNest::get_similar!";
        return;
    }
    QDomNodeList list = doc.elementsByTagName( "similar" );
    if( list.size() != 1 )
    {
        debug() << "Got no similar artists! Bailing!";
        return;
    }
    QDomElement similar = list.at( 0 ).toElement();

    // ok we have the list, now figure out what we've got in the collection

    m_collection = CollectionManager::instance()->primaryCollection();
    if( !m_collection )
        return;
    m_qm = m_collection->queryMaker();

    if( !m_qm ) // maybe this is during startup and we don't have a QM for some reason yet
        return;

    //  debug() << "got similar artists:" << similar.values();

    m_qm->beginOr();
    QDomNodeList rel = similar.childNodes();
    for( int i = 0; i < rel.count(); i++ )
    {
        QDomNode n = rel.at( i );
        QString artistname = n.firstChildElement( "name" ).text();
        debug() << "addng related artist:" << artistname;
        m_qm->addFilter( Meta::valArtist, artistname, true, true );

    }
    m_qm->endAndOr();

    m_qm->setQueryType( QueryMaker::Custom );
    m_qm->addReturnValue( Meta::valUniqueId );
    m_qm->orderByRandom(); // as to not affect the amortized time

    connect( m_qm, SIGNAL( newResultReady( QString, QStringList ) ),
             SLOT( updateReady( QString, QStringList ) ), Qt::DirectConnection );
             connect( m_qm, SIGNAL( queryDone() ), SLOT( updateFinished() ), Qt::DirectConnection );

    collectionUpdated(); // force an update
    m_qm->run();
    job->deleteLater();
}

void
Dynamic::EchoNestBias::updateFinished()
{
    DEBUG_BLOCK

    m_needsUpdating = false;
    //  emit biasUpdated( this );
}

void
Dynamic::EchoNestBias::collectionUpdated()
{
    m_needsUpdating = true;
}

void
Dynamic::EchoNestBias::updateReady( QString collectionId, QStringList uids )
{
    DEBUG_BLOCK

    Q_UNUSED(collectionId)
    QMutexLocker locker( &m_mutex );

    int protocolLength =
    ( QString( m_collection->uidUrlProtocol() ) + "://" ).length();

    QString key;
    if( m_currentOnly )
        key = m_currentArtist;
    else // save as keyed to list of artists
        key = m_currentPlaylist.join( "|" );

    //debug() << "saving found similar tracks to key:" << key;
    //debug() << "current playlist:" << m_currentPlaylist;
    m_savedArtists[ key ].clear();
    m_savedArtists[ key ].reserve( uids.size() );
    QByteArray uid;
    foreach( const QString &uidString, uids )
    {
        uid = uidString.mid( protocolLength ).toAscii();
        m_savedArtists[ key ].insert( uid );
    }
}


bool
Dynamic::EchoNestBias::trackSatisfies( const Meta::TrackPtr track )
{
    //DEBUG_BLOCK
    QMutexLocker locker( &m_mutex );

    //debug() << "checking if " << track->name() << "by" << track->artist()->name() << "is in suggested:" << m_savedArtists[ m_currentArtist ] << "of" << m_currentArtist;
    const QString uidString = track->uidUrl().mid( track->uidUrl().lastIndexOf( '/' ) );
    const QByteArray uid = uidString.toAscii();

    QString key;
    if( m_currentOnly )
        key = m_currentArtist;
    else // save as keyed to list of artists
        key = m_currentPlaylist.join( "|" );
    if( m_savedArtists.keys().contains( key ) )
    {
        debug() << "saying:" <<  m_savedArtists[ key ].contains( uid ) << "for" << track->artist()->name();
        return m_savedArtists[ key ].contains( uid );
    } else
        debug() << "DIDN'T HAVE ARTIST SUGGESTIONS SAVED FOR THIS ARTIST:" << m_currentArtist;

    return false;

}

double
Dynamic::EchoNestBias::numTracksThatSatisfy( const Meta::TrackList& tracks )
{
    DEBUG_BLOCK
    QMutexLocker locker( &m_mutex );

    int satisfy = 0;
    QString key;
    if( m_currentOnly )
        key = m_currentArtist;
    else // save as keyed to list of artists
        key = m_currentPlaylist.join( "|" );
    if( m_savedArtists.keys().contains( key ) )
    {
        foreach( const Meta::TrackPtr track, tracks )
        {
            const QString uidString = track->uidUrl().mid( track->uidUrl().lastIndexOf( '/' ) );
            const QByteArray uid = uidString.toAscii();

            if( m_savedArtists[ key ].contains( uid ) )
                satisfy++;

        }
    } else
        debug() << "AGAIN, didn't have artist suggestions saved for these multiple artists";

    return satisfy;

}

QDomElement
Dynamic::EchoNestBias::xml( QDomDocument doc ) const
{
    Q_UNUSED( doc )
    DEBUG_BLOCK

    return QDomElement();
}

// this method shamelessly inspired by liblastfm/src/ws/ws.cpp
KUrl Dynamic::EchoNestBias::createUrl( QString method, QMultiMap< QString, QString > params )
{

    params.insert( "api_key", "DD9P0OV9OYFH1LCAE" );
    params.insert( "version", "3" );

    debug() << "got param map:" << params;

    KUrl url;
    url.setScheme( "http" );
    url.setHost( "developer.echonest.com" );
    url.setPath( "/api/" + method );

    // take care of the ID possibility  manually
    // Qt setQueryItems doesn't encode a bunch of stuff, so we do it manually
    QMapIterator<QString, QString> i( params );
    while ( i.hasNext() ) {
        i.next();
        QByteArray const key = QUrl::toPercentEncoding( i.key() );
        QByteArray const value = QUrl::toPercentEncoding( i.value() );
        url.addEncodedQueryItem( key, value );
    }

    debug() << "created url for EchoNest request:" << url;

    return url;
}



bool
Dynamic::EchoNestBias::hasCollectionFilterCapability()
{
    return true;
}

Dynamic::CollectionFilterCapability*
Dynamic::EchoNestBias::collectionFilterCapability( double weight )
{
    DEBUG_BLOCK
    debug() << "returning new cfb with weight:" << weight;
    return new Dynamic::EchoNestBiasCollectionFilterCapability( this, weight );
}

const QSet< QByteArray >&
Dynamic::EchoNestBiasCollectionFilterCapability::propertySet()
{
    QString key;
    if( m_bias->m_currentOnly )
        key = m_bias->m_currentArtist;
    else // save as keyed to list of artists
        key = m_bias->m_currentPlaylist.join( "|" );
    debug() << "returning matching set for artist: " << key << "of size:" << m_bias->m_savedArtists[ key ].size();
    return m_bias->m_savedArtists[ key ];
}

double Dynamic::EchoNestBiasCollectionFilterCapability::weight() const
{
    return m_weight;
}

void
Dynamic::EchoNestBias::selectionChanged( int which )
{
    if( m_fieldSelection->itemData( which ).toString() == "playlist" )
        m_currentOnly = false;
    else
        m_currentOnly = true;
    engineNewTrackPlaying(); // refresh

}


#include "EchoNest.moc"

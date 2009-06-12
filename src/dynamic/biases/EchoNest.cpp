/**************************************************************************
* copyright            : (C) 2009 Leo Franchi <lfranchi@kde.org>          *
**************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "EchoNest.h"

#include "Collection.h"
#include "CollectionManager.h"
#include "Debug.h"
#include "EngineController.h"
#include "Meta.h"
#include "QueryMaker.h"

#include <kio/job.h>

#include <QDomDocument>
#include <QVBoxLayout>
#include <QFrame>
#include <QLabel>

// CUSTOM BIAS FACTORY

Dynamic::EchoNestBiasFactory::EchoNestBiasFactory()
: CustomBiasFactory()
{
    
}

Dynamic::EchoNestBiasFactory::~EchoNestBiasFactory()
{
    
}

QString
Dynamic::EchoNestBiasFactory::name() const
{
    
    return i18n( "EchoNest Similar Artists" );
}

QString
Dynamic::EchoNestBiasFactory::pluginName() const
{
    return "echonest_similarartists";
}



Dynamic::CustomBiasEntry*
Dynamic::EchoNestBiasFactory::newCustomBias( double weight )
{
    debug() << "CREATING ECHONEST BIAS";
    return new EchoNestBias( weight );
}

Dynamic::CustomBiasEntry*
Dynamic::EchoNestBiasFactory::newCustomBias( QDomElement e, double weight )
{
    // we don't save anything, so just load a fresh one
    Q_UNUSED( e )
    debug() << "CREATING ECHONEST BIAS 2";
    return new EchoNestBias( weight );
}


/// class SimilarArtistsBias

Dynamic::EchoNestBias::EchoNestBias( double weight )
: Dynamic::CustomBiasEntry( weight )
, EngineObserver( The::engineController() )
, m_artistNameQuery( 0 )
, m_artistSuggestedQuery( 0 )
, m_qm( 0 )
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
    QHBoxLayout* layout = new QHBoxLayout( frame );
    
    QLabel * label = new QLabel( i18n( "Adds songs related to currently playing track, recommended by EchoNest." ), parent );
    label->setWordWrap( true );
    label->setAlignment( Qt::AlignCenter );
    layout->addWidget( label, Qt::AlignCenter );
    
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
        if( !m_savedArtists.contains( track->artist()->name() ) )
        {
            QMap< QString, QString > params;
            
            params[ "query" ] = m_currentArtist;
            params[ "exact" ] = 'Y';
            params[ "sounds_like" ] = 'N'; // mutually exclusive with exact

            m_artistNameQuery = KIO::storedGet( createUrl( "search_artists", params ) );
            
            connect( m_artistNameQuery, SIGNAL( result( KJob* ) ), this, SLOT( artistNameQueryDone( KJob* ) ) );
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
    if( job != m_artistNameQuery )
    {
        debug() << "job was deleted before the slot was called..wtf?";
        return;
    }

    QDomDocument doc;
    if( ! doc.setContent( m_artistNameQuery->data() ) )
    {
        debug() << "Got invalid XML from EchoNest on artist name query!";
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

        m_artistId = id;

        // now do our second query
        QMap< QString, QString > params;
        params[ "id" ] = id;
        
        m_artistSuggestedQuery = KIO::storedGet( createUrl( "get_similar", params ) );
        
        connect( m_artistSuggestedQuery, SIGNAL( result( KJob* ) ), this, SLOT( artistSuggestedQueryDone( KJob* ) ) );

        job->deleteLater();
    }
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
        //debug() << "addng related artist:" << artistname;
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
    
    //  debug() << "setting cache of related artist UIDs for artist:" << m_currentArtist << "to:" << uids;
    m_savedArtists[ m_currentArtist ].clear();
    m_savedArtists[ m_currentArtist ].reserve( uids.size() );
    QByteArray uid;
    foreach( const QString &uidString, uids )
    {
        uid = QByteArray::fromHex( uidString.mid(protocolLength).toAscii() );
        m_savedArtists[ m_currentArtist ].insert( uid );
    }
}


bool
Dynamic::EchoNestBias::trackSatisfies( const Meta::TrackPtr track )
{
    //DEBUG_BLOCK
    QMutexLocker locker( &m_mutex );
    
    //debug() << "checking if " << track->name() << "by" << track->artist()->name() << "is in suggested:" << m_savedArtists[ m_currentArtist ] << "of" << m_currentArtist;
    QString uidString = track->uidUrl().mid( track->uidUrl().lastIndexOf( '/' ) );
    QByteArray uid = QByteArray::fromHex( uidString.toAscii() );
    
    if( m_savedArtists.keys().contains( m_currentArtist ) )
    {
        debug() << "saying:" <<  m_savedArtists[ m_currentArtist ].contains( uid ) << "for" << track->artist()->name();
        return m_savedArtists[ m_currentArtist ].contains( uid );
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
    if( m_savedArtists.keys().contains( m_currentArtist ) )
    {
        foreach( const Meta::TrackPtr track, tracks )
        {
            QString uidString = track->uidUrl().mid( track->uidUrl().lastIndexOf( '/' ) );
            QByteArray uid = QByteArray::fromHex( uidString.toAscii() );
            
            if( m_savedArtists[ m_currentArtist ].contains(uid ) )
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
KUrl Dynamic::EchoNestBias::createUrl( QString method, QMap< QString, QString > params )
{

    params[ "api_key" ] = "DD9P0OV9OYFH1LCAE";
    params[ "version" ] = '3';
    

    KUrl url;
    url.setScheme( "http" );
    url.setHost( "developer.echonest.com" );
    url.setPath( "/api/" + method );
    
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
Dynamic::EchoNestBias::collectionFilterCapability()
{
    DEBUG_BLOCK
    debug() << "returning new cfb with weight:" << weight();
    return new Dynamic::EchoNestBiasCollectionFilterCapability( this );
}

const QSet< QByteArray >&
Dynamic::EchoNestBiasCollectionFilterCapability::propertySet()
{
    debug() << "returning matching set for artist: " << m_bias->m_currentArtist << "of size:" << m_bias->m_savedArtists[ m_bias->m_currentArtist ].size();
    return m_bias->m_savedArtists[ m_bias->m_currentArtist ];
}

double Dynamic::EchoNestBiasCollectionFilterCapability::weight() const
{
    return m_bias->weight();
}

#include "EchoNest.moc"

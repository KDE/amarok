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

#include "SimilarArtistsBias.h"

#include "Collection.h"
#include "CollectionManager.h"
#include "Debug.h"
#include "EngineController.h"
#include "Meta.h"
#include "QueryMaker.h"

#include "lastfm/Artist"
#include "lastfm/ws.h"
#include "lastfm/XmlQuery"

#include <QDomDocument>
#include <QVBoxLayout>
#include <QFrame>
#include <QLabel>

Dynamic::SimilarArtistsBias::SimilarArtistsBias()
    : Dynamic::CustomBiasEntry()
    , EngineObserver( The::engineController() )
    , m_artistQuery( 0 )
{
    DEBUG_BLOCK
    engineNewTrackPlaying(); // kick it into gear if a track is already playnig. if not, it's harmless
}

Dynamic::SimilarArtistsBias::~SimilarArtistsBias()
{
    delete m_qm;
}


QString
Dynamic::SimilarArtistsBias::name()
{
    DEBUG_BLOCK
    
    return i18n( "Similar Artists" );
}

QString
Dynamic::SimilarArtistsBias::pluginName()
{
    return "lastfm_similarartists";
}


QWidget*
Dynamic::SimilarArtistsBias::configWidget( QWidget* parent )
{
    DEBUG_BLOCK
    QFrame * frame = new QFrame( parent );
    QHBoxLayout* layout = new QHBoxLayout( frame );

    QLabel * label = new QLabel( i18n( "Adds songs related to currently playing track" ), parent );
    label->setWordWrap( true );
    label->setAlignment( Qt::AlignCenter );
    layout->addWidget( label, Qt::AlignCenter );

    return frame;
}

void
Dynamic::SimilarArtistsBias::engineNewTrackPlaying()
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
            params[ "method" ] = "artist.getSimilar";
            //   params[ "limit" ] = "70";
            params[ "artist" ] = m_currentArtist;

            m_artistQuery = lastfm::ws::get( params );

            connect( m_artistQuery, SIGNAL( finished() ), this, SLOT( artistQueryDone() ) );
        }
    }
}

void
Dynamic::SimilarArtistsBias::update()
{
    DEBUG_BLOCK
    if( !m_needsUpdating )
        return;

    m_qm->run();
}

void
Dynamic::SimilarArtistsBias::artistQueryDone() // slot
{
    DEBUG_BLOCK

    if( !m_artistQuery )
    {
        debug() << "job was deleted from under us...wtf! blame the gerbils.";
        return;
    }

    QMutexLocker locker( &m_mutex );
    
    QMap< int, QString > similar =  lastfm::Artist::getSimilar( m_artistQuery );
    // ok we have the list, now figure out what we've got in the collection

    m_collection = CollectionManager::instance()->primaryCollection();
    if( !m_collection )
        return;
    m_qm = m_collection->queryMaker();
    
    if( !m_qm ) // maybe this is during startup and we don't have a QM for some reason yet
        return;

  //  debug() << "got similar artists:" << similar.values();

    m_qm->beginOr();
    foreach( QString artist, similar.values() )
    {
        m_qm->addFilter( Meta::valArtist, artist, true, true );
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
    m_artistQuery->deleteLater();
}

void
Dynamic::SimilarArtistsBias::updateFinished()
{
    DEBUG_BLOCK

    m_needsUpdating = false;
  //  emit biasUpdated( this );
}

void
Dynamic::SimilarArtistsBias::collectionUpdated()
{
    m_needsUpdating = true;
}

void
Dynamic::SimilarArtistsBias::updateReady( QString collectionId, QStringList uids )
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
Dynamic::SimilarArtistsBias::trackSatisfies( const Meta::TrackPtr track )
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
Dynamic::SimilarArtistsBias::numTracksThatSatisfy( const Meta::TrackList& tracks )
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
Dynamic::SimilarArtistsBias::xml( QDomDocument doc ) const
{
    DEBUG_BLOCK

    return QDomElement();
}


bool
Dynamic::SimilarArtistsBias::hasCollectionFilterCapability()
{
    return true;
}

Dynamic::CollectionFilterCapability*
Dynamic::SimilarArtistsBias::collectionFilterCapability()
{
    return new Dynamic::LastFmCollectionFilterCapability( this );
}

const QSet< QByteArray >&
Dynamic::LastFmCollectionFilterCapability::propertySet()
{
    debug() << "returning matching set for artist: " << m_bias->m_currentArtist << "of size:" << m_bias->m_savedArtists[ m_bias->m_currentArtist ].size();
    return m_bias->m_savedArtists[ m_bias->m_currentArtist ];
}

#include "SimilarArtistsBias.moc"

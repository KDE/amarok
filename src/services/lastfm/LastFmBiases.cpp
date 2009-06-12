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

#include "LastFmBiases.h"

#include "Collection.h"
#include "CollectionManager.h"
#include "Debug.h"
#include "EngineController.h"
#include "Meta.h"
#include "QueryMaker.h"

#include "lastfm/Artist"
#include "lastfm/ws.h"
#include "lastfm/XmlQuery"

Dynamic::LastFmBias::LastFmBias()
    : Dynamic::CustomBiasEntry()
    , EngineObserver( The::engineController() )
    , m_artistQuery( 0 )
{
    
}
/*
Dynamic::LastFmBias::~LastFmBias()
{

}*/


QString
Dynamic::LastFmBias::name()
{
    DEBUG_BLOCK
    
    return i18n( "LastFm Biases" );
}

QWidget*
Dynamic::LastFmBias::configWidget()
{
    DEBUG_BLOCK
    return new QWidget();
}

void
Dynamic::LastFmBias::engineNewTrackPlaying()
{
    DEBUG_BLOCK
    Meta::TrackPtr track = The::engineController()->currentTrack();

    if( track && track->artist() && !track->artist()->name().isEmpty() && !m_savedArtists.contains( track->artist()->name() ) )
    {
        m_currentArtist = track->artist()->name();
        QMap< QString, QString > params;
        params[ "method" ] = "artist.getSimilar";
        params[ "limit" ] = "70";
        params[ "artist" ] = m_currentArtist;

        m_artistQuery = lastfm::ws::get( params );

        connect( m_artistQuery, SIGNAL( finished() ), this, SLOT( artistQueryDone() ) );
    }
}

void
Dynamic::LastFmBias::update()
{
    DEBUG_BLOCK
    if( !m_needsUpdating )
        return;

    m_qm->run();
}

const QSet< QByteArray >& Dynamic::LastFmBias::propertySet()
{
    if( m_savedArtists.keys().contains( m_currentArtist ) )
        return m_savedArtists[ m_currentArtist ];
    else
    { // we don't want to return a reference to a temporary, so just create a stub entry
        return m_savedArtists[ "foobar" ]; // default-constructed value of this QSet should
                                           // just  be a QByteArray(), which should be safe.
    }
}


void
Dynamic::LastFmBias::artistQueryDone() // slot
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

    if( !m_collection )
        m_collection = CollectionManager::instance()->primaryCollection();
    m_qm = m_collection->queryMaker();
    
    foreach( QString artist, similar.values() )
    {
        m_qm->addFilter( Meta::valArtist, artist );
    }
    
    m_qm->setQueryType( QueryMaker::Custom );
    m_qm->addReturnValue( Meta::valUniqueId );
    m_qm->orderByRandom(); // as to not affect the amortized time

    connect( m_qm, SIGNAL( newResultReady( QString, QStringList ) ),
            SLOT( updateReady( QString, QStringList ) ), Qt::DirectConnection );
    connect( m_qm, SIGNAL( queryDone() ), SLOT( updateFinished() ), Qt::DirectConnection );

    collectionUpdated(); // force an update
    m_artistQuery->deleteLater();
}

void
Dynamic::LastFmBias::updateFinished()
{
    DEBUG_BLOCK
    
  //  emit biasUpdated( this );
}

void
Dynamic::LastFmBias::collectionUpdated()
{
    m_needsUpdating = true;
}

void
Dynamic::LastFmBias::updateReady( QString collectionId, QStringList uids )
{
    DEBUG_BLOCK

    Q_UNUSED(collectionId)
    QMutexLocker locker( &m_mutex );

    int protocolLength =
        ( QString( m_collection->uidUrlProtocol() ) + "://" ).length();

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
Dynamic::LastFmBias::trackSatisfies( const Meta::TrackPtr track )
{
    //DEBUG_BLOCK
    QMutexLocker locker( &m_mutex );

    //debug() << "checking if " << track->name() << "by" << track->artist()->name() << "is in suggested:" << m_savedArtists[ m_currentArtist ] << "of" << m_currentArtist;
    QString uidString = track->uidUrl().mid( track->uidUrl().lastIndexOf( '/' ) );
    QByteArray uid = QByteArray::fromHex( uidString.toAscii() );
    
    if( m_savedArtists.keys().contains( m_currentArtist ) )
        return m_savedArtists[ m_currentArtist ].contains( uid );
    else
        debug() << "DIDN'T HAVE ARTIST SUGGESTIONS SAVED FOR THIS ARTIST";

    return false;
    
}

double
Dynamic::LastFmBias::numTracksThatSatisfy( const Meta::TrackList& tracks )
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

#include "LastFmBiases.moc"

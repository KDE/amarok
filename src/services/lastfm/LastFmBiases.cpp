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

#include "Debug.h"
#include "EngineController.h"
#include "Meta.h"

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

    if( track && !track->artist().isNull() )
    {
        m_currentArtist = track->artist();
        QMap< QString, QString > params;
        params[ "method" ] = "artist.getSimilar";
        params[ "limit" ] = "70";
        params[ "artist" ] = m_currentArtist;

        m_artistQuery = lastfm::ws::get( params );

        connect( m_artistQuery, SIGNAL( finished() ), this, SLOT( artistQueryDone() ) );
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

    QMap< int, QString > similar =  lastfm::Artist::getSimilar( m_artistQuery );
    // for now we are ignoring the similarity taking as yes/no
    m_savedArtists[ m_currentArtist ] = similar.values();

    m_artistQuery->deleteLater();
}


bool
Dynamic::LastFmBias::trackSatisfies( const Meta::TrackPtr track )
{
    DEBUG_BLOCK

    debug() << "checking if " << track->name() << "by" << track->artist()->name() << "is in suggested:" << m_savedArtists[ m_currentArtist ] << "of" << m_currentArtist;
    if( m_savedArtists.keys().contains( m_currentArtist ) )
        return m_savedArtists[ m_currentArtist ].contains( track->artist()->name() );
    else
        debug() << "DIDN'T HAVE ARTIST SUGGESTIONS SAVED FOR THIS ARTIST";

    return false;
    
}

double
Dynamic::LastFmBias::numTracksThatSatisfy( const Meta::TrackList& tracks )
{
    DEBUG_BLOCK

    int satisfy = 0;
    if( m_savedArtists.keys().contains( m_currentArtist ) )
    {
        foreach( const Meta::TrackPtr track, tracks )
        {
            if( m_savedArtists[ m_currentArtist ].contains( track->artist()->name() ) )
                satisfy++;

        }
    } else
        debug() << "AGAIN, didn't have artist suggestions saved for these multiple artists";

    return satisfy;

}

#include "LastFmBiases.moc"

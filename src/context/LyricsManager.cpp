/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "LyricsManager.h"

#include "debug.h"
#include "enginecontroller.h"

#include <QDomDocument>


////////////////////////////////////////////////////////////////
//// CLASS LyricsObserver
///////////////////////////////////////////////////////////////

LyricsObserver::LyricsObserver()
: m_subject( 0 )
{}

LyricsObserver::LyricsObserver( LyricsSubject *s )
: m_subject( s )
{
    m_subject->attach( this );
}

LyricsObserver::~LyricsObserver()
{
    if( m_subject )
        m_subject->detach( this );
}

////////////////////////////////////////////////////////////////
//// CLASS LyricsSubject
///////////////////////////////////////////////////////////////

void LyricsSubject::sendNewLyrics( QStringList lyrics )
{
    DEBUG_BLOCK
    debug() << "sending lyrics!";
    debug() << "observers:" << m_observers;
    foreach( LyricsObserver* obs, m_observers )
    {
        debug() << "got an observer to send to";
        obs->newLyrics( lyrics );
    }
}

void LyricsSubject::sendLyricsMessage( QString msg )
{
    DEBUG_BLOCK
    debug() << "sending lyrics message:" << msg;
    foreach( LyricsObserver* obs, m_observers )
    {
        debug() << "got an observer to send to";
        obs->lyricsMessage( msg );
    }
}

void LyricsSubject::attach( LyricsObserver *obs )
{
    if( !obs || m_observers.indexOf( obs ) != -1 )
        return;
    m_observers.append( obs );
}

void LyricsSubject::detach( LyricsObserver *obs )
{
    int index = m_observers.indexOf( obs );
    if( index != -1 ) m_observers.removeAt( index );
}

////////////////////////////////////////////////////////////////
//// CLASS LyricsManager
///////////////////////////////////////////////////////////////

LyricsManager* LyricsManager::s_self = 0;

void LyricsManager::lyricsResult( QByteArray cXmlDoc, bool cached ) //SLOT
{
    DEBUG_BLOCK
    Q_UNUSED( cached );

    QDomDocument doc;
    QString xmldoc = QString::fromUtf8( cXmlDoc );
    if( !doc.setContent( xmldoc ) )
    {
    //         setData( "lyrics", "error", "error" ); // couldn't fetch
        sendLyricsMessage( QString( "error" ) );
        return;
    }

    QString lyrics;

    QDomElement el = doc.documentElement();

    if ( el.tagName() == "suggestions" )
    {

        const QDomNodeList l = doc.elementsByTagName( "suggestion" );

        if( l.length() ==0 )
        {
//             setData( "lyrics", "not found" );
            sendLyricsMessage( QString( "notfound" ) );
        }
        else
        {
            QVariantList suggested;
            for( uint i = 0; i < l.length(); ++i ) {
                const QString url    = l.item( i ).toElement().attribute( "url" );
                const QString artist = l.item( i ).toElement().attribute( "artist" );
                const QString title  = l.item( i ).toElement().attribute( "title" );

                suggested << QString( "%1 - %2 %3" ).arg( title, artist, url );
            }
//             setData( "lyrics", "suggested", suggested );
            // TODO for now suggested is disabled
        }
    } else
    {
        lyrics = el.text();
        EngineController::instance()->currentTrack()->setCachedLyrics( xmldoc); // TODO: setLyricsByPath?

        const QString title      = el.attribute( "title" );

        QStringList lyricsData;
        lyricsData << title
            << EngineController::instance()->currentTrack()->artist()->name()
            << QString() // TODO lyrics site
            << lyrics;

        debug() << "sending lyrics data:" << lyricsData;
//         setData( "lyrics", "lyrics", lyricsData );
        sendNewLyrics( lyricsData );

    }
}

#include "LyricsManager.moc"

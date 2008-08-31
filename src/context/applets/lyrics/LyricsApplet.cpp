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

#include "LyricsApplet.h"

#include "Amarok.h"
#include "Debug.h"
#include "Theme.h"

#include <QGraphicsSimpleTextItem>
#include <QGraphicsProxyWidget>
#include <QTextEdit>
#include <QPainter>
#include <QPoint>

LyricsApplet::LyricsApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_lyrics( 0 )
    , m_aspectRatio( 1 )
{
    setHasConfigurationInterface( false );
}

LyricsApplet::~ LyricsApplet()
{
    m_lyricsProxy->setWidget( 0 );
    delete m_lyricsProxy;
    m_lyricsProxy = 0;
    delete m_lyrics;
}

void LyricsApplet::init()
{
    m_lyricsProxy = new QGraphicsProxyWidget( this );
    m_lyrics = new QTextEdit;
    m_lyrics->setReadOnly( true );
    m_lyrics->setFrameShape( QFrame::NoFrame );
    m_lyricsProxy->setWidget( m_lyrics );
    m_lyrics->setPlainText( i18n( "Hello, World" ) );

    connect( dataEngine( "amarok-lyrics" ), SIGNAL( sourceAdded( const QString& ) ), this, SLOT( connectSource( const QString& ) ) );

    constraintsEvent();
}

void LyricsApplet::connectSource( const QString& source )
{
    DEBUG_BLOCK

    if( source == "lyrics" ) {
        dataEngine( "amarok-lyrics" )->connectSource( source, this );
        dataUpdated( source, dataEngine("amarok-lyrics" )->query( "lyrics" ) ); // get data initally
    }
}

void LyricsApplet::constraintsEvent( Plasma::Constraints constraints )
{
    prepareGeometryChange();

    m_lyricsProxy->setMinimumSize( size() );
    m_lyricsProxy->setMaximumSize( size() );
}

void LyricsApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    Q_UNUSED( name )
    DEBUG_BLOCK

    if( data.size() == 0 ) return;

    debug() << "lyrics applet got lyrics: " << data;

    if( data.contains( "noscriptrunning" ) )
        m_lyrics->setPlainText( i18n( "No lyrics script is running!" ) );
    if( data.contains( "fetching" ) )
        m_lyrics->setPlainText( i18n( "Lyrics are being fetched." ) );
    else if( data.contains( "error" ) )
        m_lyrics->setPlainText( i18n( "Lyrics were not able to be downloaded. Please check your internet connection." ) );
    else if( data.contains( "suggested" ) )
        m_lyrics->setPlainText( i18n( "Todo.... show suggestions here!" ) );
    else if( data.contains( "lyrics" ) )
    {
        QVariantList lyrics  = data[ "lyrics" ].toList();

        m_lyrics->setPlainText( lyrics[ 3 ].toString() );
        m_lyrics->adjustSize();
        m_lyricsProxy->resize( m_lyrics->size() );
    }
    else if( data.contains( "notfound" ) )
        m_lyrics->setPlainText( i18n( "There were no lyrics found for this track" ) );
    setPreferredSize( (int)size().width(), (int)size().height() );
}

bool LyricsApplet::hasHeightForWidth() const
{
    return false;
}

#include "LyricsApplet.moc"


QSizeF LyricsApplet::sizeHint(Qt::SizeHint which, const QSizeF & constraint) const
{
    if( constraint.height() == -1 && constraint.width() > 0 ) // asking height for given width basically
    {
        return QSizeF( constraint.width(), m_aspectRatio * constraint.width() );
    } else
    {
        return constraint;
    }
}


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
#include "debug.h"
#include "context/Svg.h"
#include "Theme.h"

#include <QGraphicsSimpleTextItem>
#include <QGraphicsProxyWidget>
#include <QTextEdit>
#include <QPainter>
#include <QPoint>

LyricsApplet::LyricsApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_header( 0 )
    , m_lyrics( 0 )
{
    Context::Theme::self()->setApplication( "amarok" );
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

    dataEngine( "amarok-lyrics" )->connectSource( "lyrics", this );

    m_header = new Context::Svg( "widgets/amarok-lyrics", this );
    m_header->setContentType( Context::Svg::SingleImage );
    m_header->resize();
    setContentSize( m_header->size() );

    m_lyricsProxy = new QGraphicsProxyWidget( this );
    m_lyrics = new QTextEdit;
    m_lyrics->setReadOnly( true );
    m_lyrics->setFrameShape( QFrame::NoFrame );
    m_lyricsProxy->setWidget( m_lyrics );

    constraintsUpdated();
}

void LyricsApplet::constraintsUpdated( Plasma::Constraints constraints )
{
    prepareGeometryChange();

    if( constraints & Plasma::SizeConstraint && m_header )
        m_header->resize(contentSize().toSize());

    QSizeF infoSize( m_header->elementRect( "lyrics" ).bottomRight().x() - m_header->elementRect( "lyrics" ).topLeft().x(),
                     m_header->elementRect( "lyrics" ).bottomRight().y() - m_header->elementRect( "lyrics" ).topLeft().y() );

    if ( infoSize.isValid() )
    {
        m_lyricsProxy->setMinimumSize( infoSize );
        m_lyricsProxy->setMaximumSize( infoSize );
    }
}

void LyricsApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    Q_UNUSED( name )
    if( data.size() == 0 ) return;

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
    setContentSize( (int)size().width(), (int)size().height() );
}

void LyricsApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option );
    Q_UNUSED( p );

    m_lyricsProxy->setPos( m_header->elementRect( "lyrics" ).topLeft() );
    m_lyricsProxy->show();
}

bool LyricsApplet::hasHeightForWidth() const
{
    return false;
}

#include "LyricsApplet.moc"

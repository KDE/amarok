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

#include "amarok.h"
#include "debug.h"
#include "context/Svg.h"

#include <QGraphicsSimpleTextItem>
#include <QGraphicsTextItem>

LyricsApplet::LyricsApplet( QObject* parent, const QStringList& args )
    : Plasma::Applet( parent, args )
    , m_theme( 0 )
    , m_size( QSizeF() )
    , m_logoAspectRatio( 0.0 )
    , m_lyrics( 0 )
    , m_title( 0 )
    , m_artist( 0 )
    , m_site( 0 )
{
    
    setHasConfigurationInterface( false );
    setDrawStandardBackground( false );
    
    dataEngine( "amarok-lyrics" )->connectSource( "lyrics", this );
    
    m_theme = new Context::Svg( "widgets/amarok-lyrics", this );
    m_theme->setContentType( Context::Svg::SingleImage );
    m_theme->resize();
    m_size = m_theme->size();
    m_logoAspectRatio = (qreal)m_theme->size().height() / 
        (qreal)m_theme->size().width();
    
    m_lyrics = new QGraphicsTextItem( this );
    m_title = new QGraphicsSimpleTextItem( this );
    m_artist = new QGraphicsSimpleTextItem( this );
    m_site = new QGraphicsSimpleTextItem( this );
    
    constraintsUpdated(); 
}

void LyricsApplet::setRect( const QRectF& rect )
{
    DEBUG_BLOCK
        
    setPos( rect.topLeft() );
    m_size = rect.size();
    resize( rect.width(), m_logoAspectRatio );
}

QSizeF LyricsApplet::contentSize() const
{
    return m_size;
}

void LyricsApplet::constraintsUpdated()
{
    DEBUG_BLOCK
        
    prepareGeometryChange();
    // align items
    m_lyrics->setPos( m_theme->elementRect( "lyrics" ).topLeft() );
    m_title->setPos( m_theme->elementRect( "lyricstrackname" ).topLeft() );
    m_artist->setPos( m_theme->elementRect( "lyricsartist" ).topLeft() );
    m_site->setPos( m_theme->elementRect( "lyricslyricssite" ).topLeft() );
    
}

void LyricsApplet::updated( const QString& name, const Plasma::DataEngine::Data& data )
{
        
    Q_UNUSED( name )
    if( data.size() == 0 ) return;
    
    if( data.contains( "fetching" ) )
        m_lyrics->setPlainText( i18n( "Lyrics are being fetched." ) );
    else if( data.contains( "error" ) )
        m_lyrics->setPlainText( i18n( "Lyrics were not able to be downloaded. Please check your internet connection." ) );
    else if( data.contains( "suggested" ) )
        m_lyrics->setPlainText( i18n( "Todo.... show suggestions here!" ) );
    else if( data.contains( "lyrics" ) )
    {
        QVariantList lyrics  = data[ "lyrics" ].toList();
        m_title->setText( lyrics[ 0 ].toString() );
        m_artist->setText( lyrics[ 1 ].toString() );
        m_site->setText( lyrics[ 2 ].toString() );
        
        m_lyrics->setPlainText( lyrics[ 3 ].toString() );
    }
    update();
}

void LyricsApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option );
    
    m_theme->paint( p, contentsRect, "lyricsbackground" );
        
    // align items
//     m_lyrics->setPos( m_theme->elementRect( "lyrics" ).topLeft() );
    m_title->setPos( m_theme->elementRect( "lyricstrackname" ).topLeft() );
    m_artist->setPos( m_theme->elementRect( "lyricsartist" ).topLeft() );
    m_site->setPos( m_theme->elementRect( "lyricslyricssite" ).topLeft() );
    
}

void LyricsApplet::resize( qreal newWidth, qreal aspectRatio )
{
    qreal height = aspectRatio * newWidth;
    m_size.setWidth( newWidth );
    m_size.setHeight( height );
    
    m_theme->resize( m_size );
    
    constraintsUpdated();
}

#include "LyricsApplet.moc"

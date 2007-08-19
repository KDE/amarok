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

#include "WikipediaApplet.h"

#include "amarok.h"
#include "debug.h"
#include "context/Svg.h"

#include <QGraphicsTextItem>
#include <QGraphicsSimpleTextItem>

WikipediaApplet::WikipediaApplet( QObject* parent, const QStringList& args )
    : Plasma::Applet( parent, args )
    , m_theme( 0 )
    , m_header( 0 )
    , m_aspectRatio( 0 )
    , m_headerAspectRatio( 0.0 )
    , m_size( QSizeF() )
    , m_wikipediaLabel( 0 )
    , m_currentLabel( 0 )
    , m_wikiPage( 0 )
{
    
    setHasConfigurationInterface( false );
    setDrawStandardBackground( true );
    
    dataEngine( "amarok-wikipedia" )->connectSource( "wikipedia", this );
    
    m_theme = new Context::Svg( "widgets/amarok-wikipedia", this );
    m_theme->setContentType( Context::Svg::SingleImage );
    m_theme->resize();
    m_aspectRatio = (qreal)m_theme->size().height()
        / (qreal)m_theme->size().width();
    m_size = m_theme->size();
    
    m_header = new Context::Svg( "widgets/amarok-wikipediaheader", this );
    m_header->setContentType( Context::Svg::SingleImage );
    m_header->resize();
    m_headerAspectRatio = (qreal)m_header->size().height()
        / (qreal)m_header->size().width();
    
    m_wikipediaLabel = new QGraphicsSimpleTextItem( this );
    m_currentLabel = new QGraphicsSimpleTextItem( this );
    
    m_wikiPage = new QGraphicsTextItem( this );
    
    QFont labelFont;
    labelFont.setBold( true );
    labelFont.setPointSize( labelFont.pointSize() + 3 );
    m_wikipediaLabel->setBrush( Qt::white );
    m_wikipediaLabel->setFont( labelFont );
    m_wikipediaLabel->setText( i18n( "Wikipedia" ) );
    
    m_currentLabel->setBrush( Qt::white );
    labelFont.setBold( false );
    m_currentLabel->setFont( labelFont );
    
    m_wikiPage->setDefaultTextColor( Qt::white );
    
    constraintsUpdated();
}

void WikipediaApplet::setRect( const QRectF& rect )
{
    setPos( rect.topLeft() );
    m_size = rect.size();
    resize( rect.width(), m_aspectRatio );
}

QSizeF WikipediaApplet::contentSize() const
{
    return m_size;
}

void WikipediaApplet::constraintsUpdated()
{
    prepareGeometryChange();
    
    m_wikipediaLabel->setPos( m_header->elementRect( "wikipedialabel" ).topLeft() );
    m_currentLabel->setPos( m_header->elementRect( "currentlabel" ).topLeft() );
    
    m_wikiPage->setPos( m_header->elementRect( "wikipediainformation" ).topLeft() );
    
    calculateHeight();
}

void WikipediaApplet::updated( const QString& name, const Plasma::DataEngine::Data& data ) // SLOT
{
    DEBUG_BLOCK
    Q_UNUSED( name )
    
    debug() << "got data from engine:" << data;
    if( data.size() == 0 ) return;
    
    if( data.contains( "message" ) )
        m_wikiPage->setPlainText( data[ "message" ].toString() );
    else
    {
        m_currentLabel->setText( data.keys()[ 0 ] ); // set type of data
        m_wikiPage->setHtml( data[ data.keys()[ 0 ] ].toString() ); // set data
    }
    update();
}

void WikipediaApplet::paintInterface(  QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option );
    
    m_theme->paint( p, contentsRect, "background" );
    QRect headerRect( 0.0, 0.0, contentsRect.width(), 0.0 );
    headerRect.setHeight( contentsRect.width() * m_headerAspectRatio );
    debug() << "header rect:" << headerRect;
    m_header->resize( headerRect.size() );
    m_header->paint( p, headerRect, "header" );
    
    calculateHeight();
}

void WikipediaApplet::calculateHeight()
{
    DEBUG_BLOCK
    qreal textHeight = m_wikiPage->boundingRect().height();
    qreal boxHeight = m_theme->size().height() - m_header->size().height();
    
     debug() << "checking if wiki text is too long for box:"
        << textHeight
        << boxHeight;
    if( textHeight > boxHeight ) // too short
    {
        qreal expandBy = textHeight - boxHeight;
        debug() << "expanding by:" << expandBy;
        m_size.setHeight( m_size.height() + expandBy );
    } /*else if( lyricsheight < m_theme->elementRect( "lyrics" ).height() )
    { // too long
        qreal shrinkBy = m_theme->elementRect( "lyrics" ).height() - lyricsheight;
        debug() << "shrinking by:" << shrinkBy
            << "final height:" << m_size.height() - shrinkBy;
        m_size.setHeight( m_size.height() - shrinkBy );
    }*/
    
    m_theme->resize( m_size );    
//     emit changed();
    debug() << "newheight:" << m_size.height();
}

void WikipediaApplet::resize( qreal newWidth, qreal aspectRatio )
{
    qreal height = aspectRatio * newWidth;
    m_size.setWidth( newWidth );
    m_size.setHeight( height );
    
    calculateHeight();
    
    debug() << "setting size to:" << m_size;
    m_theme->resize( m_size );
    m_wikiPage->setTextWidth( m_header->elementRect( "wikipediainformation" ).width() );
    constraintsUpdated();
}

#include "WikipediaApplet.moc"

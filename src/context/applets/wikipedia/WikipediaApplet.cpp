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

#include "Amarok.h"
#include "debug.h"
#include "context/Svg.h"
#include <plasma/theme.h>

#include <QGraphicsTextItem>
#include <QGraphicsSimpleTextItem>
#include <QPainter>

WikipediaApplet::WikipediaApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_theme( 0 )
    , m_header( 0 )
    , m_aspectRatio( 0 )
    , m_headerAspectRatio( 0.0 )
    , m_size( QSizeF() )
    , m_wikipediaLabel( 0 )
    , m_wikiPage( 0 )
{

    setHasConfigurationInterface( false );

}

WikipediaApplet::~ WikipediaApplet()
{
    //hacky stuff to keep QWebView from causing a crash
    m_wikiPage->setWidget( 0 );
    delete m_wikiPage;
    m_wikiPage = 0;
    delete m_webView;
}

void WikipediaApplet::init()
{

    dataEngine( "amarok-wikipedia" )->connectSource( "wikipedia", this );

    m_header = new Context::Svg( "widgets/amarok-wikipedia", this );
    m_header->setContentType( Context::Svg::SingleImage );

    m_header->resize();
    m_aspectRatio = (qreal)m_header->size().height()
        / (qreal)m_header->size().width();
    m_size = m_header->size();

    m_wikipediaLabel = new QGraphicsSimpleTextItem( this );

    m_webView = new QWebView();
    m_wikiPage = new QGraphicsProxyWidget( this );
    m_wikiPage->setWidget( m_webView );

    QFont labelFont;
    labelFont.setBold( true );
    labelFont.setPointSize( labelFont.pointSize() + 3 );
    m_wikipediaLabel->setBrush( Plasma::Theme::self()->textColor() );
    m_wikipediaLabel->setFont( labelFont );
    m_wikipediaLabel->setText( i18n( "Wikipedia" ) );

    constraintsUpdated();

}

void WikipediaApplet::constraintsUpdated( Plasma::Constraints constraints )
{
    prepareGeometryChange();
    if ( constraints & Plasma::SizeConstraint && m_header )
    {
        m_header->resize(contentSize().toSize());
    }

    m_wikipediaLabel->setPos( m_header->elementRect( "wikipedialabel" ).topLeft() );
    m_wikipediaLabel->setFont( shrinkTextSizeToFit( "Wikipedia", m_header->elementRect( "wikipedialabel" ) ) );

    //center it
    float textWidth = m_wikipediaLabel->boundingRect().width();
    float totalWidth = m_header->elementRect( "wikipedialabel" ).width();
    float offsetX =  ( totalWidth - textWidth ) / 2;

    m_wikiPage->setPos( m_header->elementRect( "wikipediainformation" ).topLeft() );

    QSizeF infoSize( m_header->elementRect( "wikipediainformation" ).bottomRight().x() - m_header->elementRect( "wikipediainformation" ).topLeft().x(), m_header->elementRect( "wikipediainformation" ).bottomRight().y() - m_header->elementRect( "wikipediainformation" ).topLeft().y() );

    if ( infoSize.isValid() ) {
        m_wikiPage->setMinimumSize( infoSize );
        m_wikiPage->setMaximumSize( infoSize );
    }

    m_wikiPage->show();
}

bool WikipediaApplet::hasHeightForWidth() const
{
    return true;
}

qreal WikipediaApplet::heightForWidth( qreal width ) const
{
    return width * m_aspectRatio;;
}


void WikipediaApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data ) // SLOT
{
    Q_UNUSED( name )

    if( data.size() == 0 ) return;

    if( data.contains( "page" ) ) {
        m_webView->setHtml( data[ "page" ].toString() );
    } else {
        m_webView->setHtml( data[ data.keys()[ 0 ] ].toString() ); // set data
    }

    if( data.contains( "label" ) )
        m_label = data[ "label" ].toString() + ':';
    else
        m_label = QString();

    if( data.contains( "title" ) )
        m_title = data[ "title" ].toString();
    else
        m_title = QString();
}

void WikipediaApplet::paintInterface(  QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option );

    m_header->resize(contentSize().toSize());

    p->save();
    m_header->paint( p, contentsRect/*, "header" */);
    p->restore();
}



#include "WikipediaApplet.moc"

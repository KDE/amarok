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

#include "ServiceInfo.h"

#include "Amarok.h"
#include "debug.h"
#include "context/Svg.h"

#include <plasma/theme.h>

#include <QDBusInterface>
#include <QPainter>
#include <QBrush>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>

ServiceInfo::ServiceInfo( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_config( 0 )
    , m_configLayout( 0 )
    , m_width( 0 )
    , m_aspectRatio( 0 )
    , m_size( QSizeF() )
    , m_initialized( false )

{
    DEBUG_BLOCK

    setHasConfigurationInterface( false );

    dataEngine( "amarok-service" )->connectSource( "service", this );

    m_theme = new Context::Svg( "widgets/amarok-serviceinfo", this );
    m_theme->setContainsMultipleImages( false );
    m_theme->resize( m_size );
    m_width = globalConfig().readEntry( "width", 500 );

    m_serviceName = new QGraphicsSimpleTextItem( this );
    m_serviceName->setText( "Service Info (No service active)" );

    m_webView = new QWebView( 0 );
    m_serviceMainInfo = new QGraphicsProxyWidget( this );
    m_serviceMainInfo->setWidget( m_webView );

    m_webView->page()->setLinkDelegationPolicy ( QWebPage::DelegateAllLinks );

    connect ( m_webView->page(), SIGNAL( linkClicked ( const QUrl & ) ) , this, SLOT( linkClicked ( const QUrl & ) ) );


    m_serviceName->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );

    // get natural aspect ratio, so we can keep it on resize
    m_theme->resize();
    m_aspectRatio = (qreal)m_theme->size().height() / (qreal)m_theme->size().width();
    resize( m_width, m_aspectRatio );

    constraintsUpdated();
}

ServiceInfo::~ServiceInfo()
{
    //hacky stuff to keep QWebView from causing a crash
    m_serviceMainInfo->setWidget( 0 );
    delete m_serviceMainInfo;
    m_serviceMainInfo = 0;
    delete m_webView;

}

void ServiceInfo::constraintsUpdated( Plasma::Constraints constraints )
{

    prepareGeometryChange();

    if (constraints & Plasma::SizeConstraint && m_theme) {
        m_theme->resize(size().toSize());
    }



    //make the text as large as possible:
    m_serviceName->setFont( shrinkTextSizeToFit( m_serviceName->text(), m_theme->elementRect( "service_name" ) ) );

    //center it

    float textWidth = m_serviceName->boundingRect().width();
    float totalWidth = m_theme->elementRect( "service_name" ).width();
    float offsetX =  ( totalWidth - textWidth ) / 2;

    kDebug() << "offset: " << offsetX;

    m_serviceName->setPos( m_theme->elementRect( "service_name" ).topLeft() + QPointF ( offsetX, 0 ) );




    //QSizeF infoSize( 200, 200 );
    QSizeF infoSize( m_theme->elementRect( "main_info" ).bottomRight().x() - m_theme->elementRect( "main_info" ).topLeft().x(), m_theme->elementRect( "main_info" ).bottomRight().y() - m_theme->elementRect( "main_info" ).topLeft().y() );

    if ( infoSize.isValid() ) {
        m_serviceMainInfo->setMinimumSize( infoSize );
        m_serviceMainInfo->setMaximumSize( infoSize );
    }

    m_initialized = true;

}

void ServiceInfo::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    DEBUG_BLOCK
    Q_UNUSED( name );

    if( data.size() == 0 ) return;

    kDebug() << "got data from engine: " << data[ "service_name" ].toString();

    if  ( m_initialized ) {
        m_serviceName->setText( data[ "service_name" ].toString() );
        m_webView->setHtml( data[ "main_info" ].toString() );
        constraintsUpdated();
    }

}

void ServiceInfo::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option );

    //bail out if there is no room to paint. Prevents crashes and really there is no sense in painting if the
    //context view has been minimized completely
    if ( ( contentsRect.width() < 40 ) || ( contentsRect.height() < 40 ) ) {
        debug() << "Too little room to paint, hiding all children ( making myself invisible but still painted )!";
        foreach ( QGraphicsItem * childItem, QGraphicsItem::children() ) {
            childItem->hide();
        }
        return;
    } else {
        foreach ( QGraphicsItem * childItem, QGraphicsItem::children () ) {
            childItem->show();
        }
    }

    p->save();
    m_theme->paint( p, contentsRect/*, "background" */);
    p->restore();

    //m_serviceName->setPos( m_theme->elementRect( "service_name" ).topLeft() );
    m_serviceMainInfo->setPos( m_theme->elementRect( "main_info" ).topLeft() );

    QSizeF infoSize( m_theme->elementRect( "main_info" ).bottomRight().x() - m_theme->elementRect( "main_info" ).topLeft().x(), m_theme->elementRect( "main_info" ).bottomRight().y() - m_theme->elementRect( "main_info" ).topLeft().y() );
    //infoSize.setHeight(  infoSize.height() - 50 );

    m_serviceMainInfo->setMinimumSize( infoSize );
    m_serviceMainInfo->setMaximumSize( infoSize );
    m_serviceMainInfo->show();

}

void ServiceInfo::showConfigurationInterface()
{

}

void ServiceInfo::configAccepted() // SLOT
{

}

void ServiceInfo::resize( qreal newWidth, qreal aspectRatio )
{
    Q_UNUSED( newWidth ); Q_UNUSED( aspectRatio );
}

bool ServiceInfo::hasHeightForWidth() const
{
    return true;
}

qreal ServiceInfo::heightForWidth(qreal width) const
{
    return width * m_aspectRatio;
}

void ServiceInfo::linkClicked( const QUrl & url )
{
    kDebug() << "Link clicked: " << url.toString();

    //for now, just handle xspf playlist files

    if ( url.toString().contains( ".xspf", Qt::CaseInsensitive ) ) {

        QDBusInterface amarokPlaylist( "org.kde.amarok", "/Playlist" );
        amarokPlaylist.call( "addMedia", url.toString() );
    }
}


#include "ServiceInfo.moc"

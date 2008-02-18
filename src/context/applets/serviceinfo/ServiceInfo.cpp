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

#include "amarok.h"
#include "debug.h"
#include "context/Svg.h"

#include <QPainter>
#include <QBrush>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>

ServiceInfo::ServiceInfo( QObject* parent, const QVariantList& args )
    : Plasma::Applet( parent, args )
    , m_config( 0 )
    , m_configLayout( 0 )
    , m_width( 0 )
    , m_aspectRatio( 0 )
    , m_size( QSizeF() )

{
    DEBUG_BLOCK

    setHasConfigurationInterface( false );

    dataEngine( "amarok-service" )->connectSource( "service", this );

    m_theme = new Context::Svg( "widgets/amarok-serviceinfo", this );
    m_theme->setContentType( Context::Svg::SingleImage );
    m_theme->resize( m_size );
    m_width = globalConfig().readEntry( "width", 500 );

    m_serviceName = new QGraphicsSimpleTextItem( this );
    m_serviceName->setText( "Service Info ( No sevice Active )" );

    m_webView = new QWebView();
    m_serviceMainInfo = new QGraphicsProxyWidget( this );
    m_serviceMainInfo->setWidget( m_webView );



    m_serviceName->setBrush( QBrush( Qt::white ) );
    //m_serviceMainInfo->setBrush( QBrush( Qt::white ) );

    // get natural aspect ratio, so we can keep it on resize
    m_theme->resize();
    m_aspectRatio = (qreal)m_theme->size().height() / (qreal)m_theme->size().width();
    resize( m_width, m_aspectRatio );

    constraintsUpdated();
}

ServiceInfo::~ServiceInfo()
{

}

void ServiceInfo::constraintsUpdated( Plasma::Constraints constraints )
{

    prepareGeometryChange();

    if (constraints & Plasma::SizeConstraint && m_theme) {
        m_theme->resize(contentSize().toSize());
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

}

void ServiceInfo::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    DEBUG_BLOCK
    Q_UNUSED( name );

    if( data.size() == 0 ) return;

    kDebug() << "got data from engine: " << data[ "service_name" ].toString();
    m_serviceName->setText( data[ "service_name" ].toString() );
    m_webView->setHtml( data[ "main_info" ].toString() );

}

void ServiceInfo::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option );

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

}

bool ServiceInfo::hasHeightForWidth() const
{
    return true;
}

qreal ServiceInfo::heightForWidth(qreal width) const
{
    return width * m_aspectRatio;
}

QFont ServiceInfo::shrinkTextSizeToFit( const QString& text, const QRectF& bounds )
{

    int size = 30; // start here, shrink if needed
    QFont font( QString(), size, QFont::Light );
    font.setStyleHint( QFont::SansSerif );
    font.setStyleStrategy( QFont::PreferAntialias );

    QFontMetrics fm( font );
    while( fm.height() > bounds.height() + 4 )
    {
        if( size < 0 )
        {
            size = 5;
            break;
        }
        size--;
        fm = QFontMetrics( QFont( QString(), size ) );
    }
    
    // for aesthetics, we make it one smaller
    size--;

    QFont returnFont( QString(), size, QFont::Light );
    font.setStyleHint( QFont::SansSerif );
    font.setStyleStrategy( QFont::PreferAntialias );
    
    return QFont( returnFont );
}

#include "ServiceInfo.moc"

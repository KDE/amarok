/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 * copyright   : (C) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>   *
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
#include "Debug.h"
#include "context/Svg.h"
#include "playlist/PlaylistModel.h"

#include <plasma/theme.h>

#include <KStandardDirs>

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
    , m_currentPlaylist( 0 )

{
    DEBUG_BLOCK

    setHasConfigurationInterface( false );
    setBackgroundHints( Plasma::Applet::NoBackground );

    dataEngine( "amarok-service" )->connectSource( "service", this );

    m_theme = new Plasma::PanelSvg( this );
    QString imagePath = KStandardDirs::locate("data", "amarok/images/web_applet_background.svg" );

    kDebug() << "Loading theme file: " << imagePath;

    m_theme->setImagePath( imagePath );
    m_theme->setContainsMultipleImages( true );
    m_theme->setEnabledBorders( Plasma::PanelSvg::AllBorders );

    m_header = new Context::Svg( this );
    m_header->setImagePath( "widgets/amarok-serviceinfo" );
    m_header->setContainsMultipleImages( false );
    m_header->resize( m_size );
    m_width = globalConfig().readEntry( "width", 500 );

    m_serviceName = new QGraphicsSimpleTextItem( this );
    m_serviceName->setText( i18n("Service Info (No service active)") );

    //m_serviceMainInfo = new QGraphicsProxyWidget( this );
    //m_webView = new QWebView( 0 );

    m_webView = new Plasma::WebContent( this );

    QPalette p = m_webView->palette();
    p.setColor( QPalette::Dark, QColor( 255, 255, 255, 0)  );
    p.setColor( QPalette::Window, QColor( 255, 255, 255, 0)  );
    m_webView->setPalette( p );

    //m_serviceMainInfo->setWidget( m_webView );

    connect ( m_webView->page(), SIGNAL( linkClicked ( const QUrl & ) ) , this, SLOT( linkClicked ( const QUrl & ) ) );

    m_serviceName->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );

    // get natural aspect ratio, so we can keep it on resize
    m_header->resize();
    m_aspectRatio = (qreal)m_header->size().height() / (qreal)m_header->size().width();
    resize( m_width, m_aspectRatio );

    constraintsEvent();
}

ServiceInfo::~ServiceInfo()
{
    //hacky stuff to keep QWebView from causing a crash
    /*m_serviceMainInfo->setWidget( 0 );
    delete m_serviceMainInfo;
    m_serviceMainInfo = 0;*/
    delete m_webView;

}

void ServiceInfo::constraintsEvent( Plasma::Constraints constraints )
{
    if( !m_header )
        return;

    prepareGeometryChange();

    if( constraints & Plasma::SizeConstraint )
        m_header->resize( size().toSize() );

    m_theme->resizePanel( size().toSize() );

    //make the text as large as possible:
    m_serviceName->setFont( shrinkTextSizeToFit( m_serviceName->text(), m_header->elementRect( "service_name" ) ) );

    //center it

    float textWidth = m_serviceName->boundingRect().width();
    float totalWidth = m_header->elementRect( "service_name" ).width();
    float offsetX =  ( totalWidth - textWidth ) / 2;

    m_serviceName->setPos( m_header->elementRect( "service_name" ).topLeft() + QPointF ( offsetX, 0 ) );

    QSizeF infoSize( m_header->elementRect( "main_info" ).bottomRight().x() - m_header->elementRect( "main_info" ).topLeft().x() - 14, m_header->elementRect( "main_info" ).bottomRight().y() - m_header->elementRect( "main_info" ).topLeft().y() - 7 );

    m_webView->resize( infoSize );

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
        m_webView->setHtml( data[ "main_info" ].toString(), KUrl( QString() ) );
        m_webView->page()->setLinkDelegationPolicy( QWebPage::DelegateAllLinks );
        constraintsEvent();
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
    
    m_header->resize(size().toSize());
    m_theme->resizePanel(size().toSize());
    p->save();
//     m_header->paint( p, contentsRect );
    m_theme->paintPanel( p, QRectF( 0.0, 0.0, size().toSize().width(), size().toSize().height() ) );
    p->restore();

    //m_serviceName->setPos( m_header->elementRect( "service_name" ).topLeft() );
    QPointF pos( m_header->elementRect( "main_info" ).topLeft() );
    //m_serviceMainInfo->setPos( pos.x() + 7, pos.y() );
    m_webView->setPos( pos.x() + 7, pos.y() );

    QSizeF infoSize( m_header->elementRect( "main_info" ).bottomRight().x() - m_header->elementRect( "main_info" ).topLeft().x() - 14, m_header->elementRect( "main_info" ).bottomRight().y() - m_header->elementRect( "main_info" ).topLeft().y() - 10 );
    //infoSize.setHeight(  infoSize.height() - 50 );

    /*m_serviceMainInfo->setMinimumSize( infoSize );
    m_serviceMainInfo->setMaximumSize( infoSize );
    m_serviceMainInfo->show();
*/
    m_webView->resize( infoSize );
    

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

QSizeF 
ServiceInfo::sizeHint( Qt::SizeHint which, const QSizeF & constraint) const
{
    if( constraint.height() == -1 && constraint.width() > 0 ) // asking height for given width basically
    {
        return QSizeF( constraint.width(), m_aspectRatio * constraint.width() );
    } else
    {
        return constraint;
    }
}

void ServiceInfo::linkClicked( const QUrl & url )
{
    debug() << "Link clicked: " << url.toString();

    //for now, just handle xspf playlist files

    if ( url.toString().contains( ".xspf", Qt::CaseInsensitive ) ) {

        Meta::XSPFPlaylist * playlist = new Meta::XSPFPlaylist( url );
        playlist->subscribe( this );

    }
}

void ServiceInfo::trackListChanged( Meta::Playlist * playlist )
{
    playlist->unsubscribe( this );
    Meta::PlaylistPtr playlistPtr( playlist );
    The::playlistModel()->insertPlaylist( The::playlistModel()->rowCount(), playlistPtr );
}


#include "ServiceInfo.moc"

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
#include "amarokurls/AmarokUrl.h"
#include "Debug.h"
#include "context/Svg.h"
#include "playlist/PlaylistController.h"

#include <plasma/theme.h>

#include <KStandardDirs>

#include <QPainter>


QString ServiceInfo::s_defaultHtml = "<html>"
                                    "    <head>"
                                    "        <style type=\"text/css\">body {text-align:center}</style>"
                                    "    </head>"
                                    "    <body>"
                                    "        <b>%%SERVICE_NAME%%</b>"
                                    "    </body>"
                                    "</html>";

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
    setHasConfigurationInterface( false );
    setBackgroundHints( Plasma::Applet::NoBackground );

    dataEngine( "amarok-service" )->connectSource( "service", this );

    m_theme = new Plasma::FrameSvg( this );
    QString imagePath = KStandardDirs::locate("data", "amarok/images/web_applet_background.svg" );

    kDebug() << "Loading theme file: " << imagePath;

    m_theme->setImagePath( imagePath );
    m_theme->setContainsMultipleImages( true );
    m_theme->setEnabledBorders( Plasma::FrameSvg::AllBorders );

    m_header = new Context::Svg( this );
    m_header->setImagePath( "widgets/amarok-serviceinfo" );
    m_header->setContainsMultipleImages( false );
    m_header->resize( m_size );
    m_width = globalConfig().readEntry( "width", 500 );

    //m_serviceMainInfo = new QGraphicsProxyWidget( this );
    //m_webView = new QWebView( 0 );

    m_webView = new Plasma::WebView( this );

    QPalette p = m_webView->palette();
    p.setColor( QPalette::Dark, QColor( 255, 255, 255, 0)  );
    p.setColor( QPalette::Window, QColor( 255, 255, 255, 0)  );
    m_webView->setPalette( p );

    //some css voodoo to make sure the background of the page is a sane color
    m_webView->page()->settings()->setUserStyleSheetUrl( "file://" + KStandardDirs::locate("data", "amarok/data/ServiceInfoCustomStyle.css" ) );
    
    //m_serviceMainInfo->setWidget( m_webView );

    connect ( m_webView->page(), SIGNAL( linkClicked ( const QUrl & ) ) , this, SLOT( linkClicked ( const QUrl & ) ) );

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

    m_theme->resizeFrame( size().toSize() );

    QSizeF infoSize( m_header->elementRect( "main_info" ).bottomRight().x() - m_header->elementRect( "main_info" ).topLeft().x() - 14, m_header->elementRect( "main_info" ).bottomRight().y() - m_header->elementRect( "main_info" ).topLeft().y() - 10 );

    m_webView->resize( infoSize );

    m_initialized = true;

}

void ServiceInfo::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    Q_UNUSED( name );

    if( data.size() == 0 ) return;

    kDebug() << "got data from engine: " << data[ "service_name" ].toString();

    if  ( m_initialized ) {


        if ( !data[ "main_info" ].toString().isEmpty() )
        {
            m_webView->setHtml( data[ "main_info" ].toString(), KUrl( QString() ) );
        }
        else
        {
            QString html = s_defaultHtml;
            html = html.replace( "%%SERVICE_NAME%%", data[ "service_name" ].toString() );
            m_webView->setHtml( html );
        }
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
    m_theme->resizeFrame(size().toSize());
    p->save();
//     m_header->paint( p, contentsRect );
    m_theme->paintFrame( p, QRectF( 0.0, 0.0, size().toSize().width(), size().toSize().height() ) );
    p->restore();

    QPointF pos( m_header->elementRect( "main_info" ).topLeft() );
    //m_serviceMainInfo->setPos( pos.x() + 7, pos.y() );
    m_webView->setPos( pos.x() + 7, pos.y() + 5 );

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
    // TODO hardcoding for now.
    // i want to have a system where an applet can ask
    // for a full "CV pane" of size, but for now this will stop the crash
    QSizeF size;
    size.setWidth( QGraphicsWidget::sizeHint( which, constraint ).width() );
    size.setHeight( 450 );
    return size;
}

void ServiceInfo::linkClicked( const QUrl & url )
{
    debug() << "Link clicked: " << url.toString();


    if ( url.toString().startsWith( "amarok://", Qt::CaseInsensitive ) ) {
        AmarokUrl aUrl( url.toString() );
        aUrl.run();
    } else if ( url.toString().contains( ".xspf", Qt::CaseInsensitive ) ) {

        Meta::XSPFPlaylist * playlist = new Meta::XSPFPlaylist( url );
        playlist->subscribe( this );

    }
}

void ServiceInfo::trackListChanged( Meta::Playlist * playlist )
{
    playlist->unsubscribe( this );
    Meta::PlaylistPtr playlistPtr( playlist );
    The::playlistController()->insertOptioned( playlistPtr, Playlist::Append );
}


#include "ServiceInfo.moc"


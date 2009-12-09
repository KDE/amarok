/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "InfoApplet.h"

#include "Amarok.h"
#include "App.h"
#include "amarokurls/AmarokUrl.h"
#include "Debug.h"
#include "PaletteHandler.h"
#include "playlist/PlaylistController.h"

#include <KStandardDirs>

#include <QPainter>
#include <QDesktopServices>

QString InfoApplet::s_defaultHtml = "<html>"
                                    "    <head>"
                                    "        <style type=\"text/css\">body {text-align:center}</style>"
                                    "    </head>"
                                    "    <body>"
                                    "        <b>%%SUBJECT_NAME%%</b>"
                                    "    </body>"
                                    "</html>";

InfoApplet::InfoApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_initialized( false )
    , m_currentPlaylist( 0 )
{
    setHasConfigurationInterface( false );
    setBackgroundHints( Plasma::Applet::NoBackground );

    dataEngine( "amarok-info" )->connectSource( "info", this );

    m_webView = new AmarokWebView( this );

    resize( 500, -1 );
    
    QPalette p = m_webView->palette();
    p.setColor( QPalette::Dark, QColor( 255, 255, 255, 0)  );
    p.setColor( QPalette::Window, QColor( 255, 255, 255, 0)  );
    m_webView->setPalette( p );

    connect( m_webView->page(), SIGNAL( linkClicked ( const QUrl & ) ), SLOT( linkClicked ( const QUrl & ) ) );

    constraintsEvent();
}

InfoApplet::~InfoApplet()
{
    delete m_webView;
}

void InfoApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints )
    
    prepareGeometryChange();

    m_webView->setPos( standardPadding(), standardPadding() );
    m_webView->resize( boundingRect().width() - 2 * standardPadding(), boundingRect().height() - 2 * standardPadding() );

    m_initialized = true;
}

void InfoApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    Q_UNUSED( name );

    if( data.isEmpty() )
        return;

    debug() << "got data from engine: " << data[ "subject_name" ].toString();

    if  ( m_initialized )
    {
        QString currentHtml = data[ "main_info" ].toString();
        if ( !currentHtml.isEmpty() )
        {
            QColor highlight( App::instance()->palette().highlight().color() );
            highlight.setHsvF( highlight.hueF(), 0.3, .95, highlight.alphaF() );
            currentHtml = currentHtml.replace( "{text_color}", App::instance()->palette().brush( QPalette::Text ).color().name() );
            currentHtml = currentHtml.replace( "{content_background_color}", highlight.name() );
            currentHtml = currentHtml.replace( "{background_color}", PaletteHandler::highlightColor().lighter( 150 ).name());
            currentHtml = currentHtml.replace( "{border_color}", PaletteHandler::highlightColor().lighter( 150 ).name() );
            
            m_webView->setHtml( currentHtml, KUrl( QString() ) );
        }
        else
        {
            currentHtml = s_defaultHtml;
            currentHtml = currentHtml.replace( "%%SUBJECT_NAME%%", data[ "subject_name" ].toString() );
            m_webView->setHtml( currentHtml );
        }

        m_webView->page()->setLinkDelegationPolicy( QWebPage::DelegateAllLinks );
        updateConstraints();
    }
}

void InfoApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{    
    Q_UNUSED( option );

    //bail out if there is no room to paint. Prevents crashes and really there is no sense in painting if the
    //context view has been minimized completely
    if ( ( contentsRect.width() < 40 ) || ( contentsRect.height() < 40 ) )
    {
        debug() << "Too little room to paint, hiding all children ( making myself invisible but still painted )!";
        foreach ( QGraphicsItem * childItem, QGraphicsItem::children() )
            childItem->hide();

        return;
    }
    else
    {
        foreach ( QGraphicsItem * childItem, QGraphicsItem::children () )
            childItem->show();
    }

    p->setRenderHint( QPainter::Antialiasing );

    addGradientToAppletBackground( p );
}

void InfoApplet::linkClicked( const QUrl & url )
{
    debug() << "Link clicked: " << url.toString();

    if ( url.toString().startsWith( "amarok://", Qt::CaseInsensitive ) )
    {
        AmarokUrl aUrl( url.toString() );
        aUrl.run();
    }
    else if ( url.toString().contains( ".xspf", Qt::CaseInsensitive ) )
    {
        new Meta::XSPFPlaylist( url, true );
    }
    else
        QDesktopServices::openUrl( url.toString() );
}

#include "InfoApplet.moc"


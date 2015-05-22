/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#define DEBUG_PREFIX "InfoApplet"

#include "InfoApplet.h"

#include "App.h"
#include "amarokurls/AmarokUrl.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "PaletteHandler.h"
#include "playlist/PlaylistController.h"

#include <KGraphicsWebView>
#include <KStandardDirs>

#include <QPainter>
#include <QDesktopServices>
#include <QGraphicsLinearLayout>

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
    , m_webView( 0 )
    , m_initialized( false )
{
    setHasConfigurationInterface( false );
}

InfoApplet::~InfoApplet()
{
    delete m_webView;
}


void InfoApplet::init()
{
    // Call the base implementation.
    Context::Applet::init();

    dataEngine( "amarok-info" )->connectSource( "info", this );

    m_webView = new KGraphicsWebView( this );

    QPalette p = m_webView->palette();
    p.setColor( QPalette::Dark, QColor( 255, 255, 255, 0)  );
    p.setColor( QPalette::Window, QColor( 255, 255, 255, 0)  );
    m_webView->setPalette( p );

    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout( Qt::Vertical, this );
    layout->addItem( m_webView );

    connect( m_webView->page(), SIGNAL(linkClicked(QUrl)), SLOT(linkClicked(QUrl)) );

    updateConstraints();
}

void InfoApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints )
    m_initialized = true;
}

void InfoApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    Q_UNUSED( name );

    if( data.isEmpty() )
        return;

    if( m_initialized )
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

void InfoApplet::linkClicked( const QUrl & url )
{
    DEBUG_BLOCK
    debug() << "Link clicked: " << url.toString();

    if ( url.toString().startsWith( "amarok://", Qt::CaseInsensitive ) )
    {
        AmarokUrl aUrl( url.toString() );
        aUrl.run();
    }
    else if ( url.toString().contains( ".xspf", Qt::CaseInsensitive ) )
    {
        // FIXME: this doesn't work (triggerTrackLoad is not called) and leaks the playlist instance
        new Playlists::XSPFPlaylist( url, 0, Playlists::XSPFPlaylist::AppendToPlaylist );
    }
    else
        QDesktopServices::openUrl( url.toString() );
}


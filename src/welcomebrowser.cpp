// (c) Max Howell 2004
// (c) Christian Muehlhaeuser 2004
// See COPYING file for licensing information


#include "app.h"
#include "amarokconfig.h"
#include "welcomebrowser.h"

#include <khtml_part.h>
#include <kdebug.h>
#include <klocale.h>


WelcomeBrowser::WelcomeBrowser( QObject* parent, const char *name )
        : QVBox( 0, name )
{
    browser = new KHTMLPart( this );
    setStyleSheet();
    showPage();

    connect( browser->browserExtension(), SIGNAL(openURLRequest( const KURL&, const KParts::URLArgs&) ),
             parent,                      SLOT(welcomeURL( const KURL& )) );
}


//////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED
//////////////////////////////////////////////////////////////////////////////////////////

void WelcomeBrowser::paletteChange( const QPalette& pal )
{
    kdDebug() << k_funcinfo << endl;

    QVBox::paletteChange( pal );

    setStyleSheet();
    showPage();
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE
//////////////////////////////////////////////////////////////////////////////////////////

void WelcomeBrowser::setStyleSheet()
{
    QFontMetrics fm( AmarokConfig::useCustomFonts() ? AmarokConfig::playlistWindowFont() : QApplication::font() );
    int pxSize = fm.height() - 3;

    m_styleSheet =  QString( "div { color: %1; font-size: %2px; text-decoration: none; }" )
                    .arg( colorGroup().text().name() ).arg( pxSize );

    m_styleSheet +=  QString( "a { color: %1; font-size: %2px; }" )
                    .arg( colorGroup().highlight().name() ).arg( pxSize );

    m_styleSheet += QString( ".title { color: %1; font-size: %2px; font-weight: bold; text-decoration: none; }" )
                    .arg( colorGroup().highlight().name() ).arg( pxSize + 2 );

    m_styleSheet += QString( ".subtitle { color: %1; font-size: %2px; font-weight: bold; text-decoration: none; }" )
                    .arg( colorGroup().text().name() ).arg( pxSize + 1 );
}


void WelcomeBrowser::showPage()
{
    browser->begin();
    browser->setUserStyleSheet( m_styleSheet );

    QString welcomeText = "<html><body>"
                          "<div class='title'>" + i18n( "Welcome to amaroK!" ) + "</div>"
                          "<div>" + i18n( "There are many media players around these days, this is true; but, what is missing from most players, is an interface that does not get in the way of the user. How many buttons do you have to press when simply adding media to the playlist? amaroK tries to be a little different, providing a simple drag-and-drop interface that makes playlist handling really easy." ) + "</div><br>"
                          "<div class='title'>" + i18n( "Customise your amaroK" ) + "</div>"
                          "<div>" + i18n( "amaroK has two main modes of operation. It can look and act a little like XMMS and other Winamp clones [SCREENIE], or it can have a single window with a statusbar to keep you up to date with the track progress. You can switch between them using the following links:" ) +
                          "<ul>"
                          "<li><A href='amarok://xmms_mode'>" + i18n( "Switch to XMMS-mode" ) + "</A>"
                          "<li><A href='amarok://compact_mode'>" + i18n( "Switch to Compact-mode" ) + "</A>"
                          "</ul>"
                          "</div><br>"
                          "<div class='title'>" + i18n( "Using amaroK" ) + "</div>"
                          "<div>" + i18n( "amaroK is all about easy playlist building. This welcome text is embedded in a tab in the BrowserBar; each tab in the bar is a route to your music and media: open the browsers by clicking the tabs and then drag media from the listings to the playlist. The different browsers are as follows:" ) +
                          "<ol>"
                          "<li><span class='subtitle'>" + i18n( "File Browser" ) + "</span><p>" + i18n( "The FileBrowser will be the most useful browser to people at first; use it to navigate your filesystem to where you keep your music and then drag-and-drop to the playlist. Filtering by <EM>*.mp3 *.ogg</EM> may prove useful to you." ) + "</p>"
                          "<li><span class='subtitle'>" + i18n( "Search Browser" ) + "</span><p>" + i18n( "Some people have too much music to remember where it all is. The Search Browser makes it easy to find lost tracks: specify a search term (eg. 'Chem' to find all your <EM>Chemical Brothers</EM> music), specify a path to start the search in, and click <STRONG>Search</STRONG>. You can even drag-and-drop the files to Konqueror afterwards if you want to move them all to a sensible location so you do not need to search again next time." ) + "</p>"
                          "<li><span class='subtitle'>" + i18n( "Collection Browser" ) + "</span><p>" + i18n( "This is amaroK's lightning-fast music database. Assemble the database and enjoy the benefits of fast, categorised music sorting." ) + "</p>"
                          "<li><span class='subtitle'>" + i18n( "Context Browser" ) + "</span><p>" + i18n( "Statistics, context, brilliance." ) + "</p>"
                          "<li><span class='subtitle'>" + i18n( "Stream Browser" ) + "</span><p>" + i18n( "Internet radio is a fabulous way to find out about new music without upsetting the various record industries of the world. Fetch the streams from the online meta-server and drag-and-drop something to suit your tastes. You can even submit your own favorite radio stations to the server to increase the choice for everyone else." ) + "</p>"
                          "</ol>"
                          "</div><br>"
                          "<div style='text-align:center'>" + i18n( "To remove this tab from the BrowserBar," ) + " <A href='amarok://remove_tab'>" + i18n( "click here" ) + "</A>.</div>"
                          "</body></html>";

    browser->write( welcomeText );
    browser->end();
}


#include "welcomebrowser.moc"

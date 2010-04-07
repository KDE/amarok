/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
 * Copyright (c) 2009 Vignesh Chandramouli <vig.chan@gmail.com>                                                                                     *
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

#include "WikiApplet.h"

#include "Amarok.h"
#include "App.h"
#include "Debug.h"
#include "context/Svg.h"
#include "EngineController.h"
#include "PaletteHandler.h"
#include "dialogs/ScriptManager.h"

#include <KGlobalSettings>
#include <plasma/theme.h>
#include <plasma/widgets/iconwidget.h>
#include <plasma/widgets/busywidget.h>

#include <KIcon>
#include <KStandardDirs>
#include <KConfigDialog>
#include <KPushButton>

#include <QAction>
#include <QMenu>
#include <QTextDocument>
#include <QDesktopServices>
#include <QGraphicsSimpleTextItem>
#include <QPainter>
#include <QWebFrame>

WikiApplet::WikiApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_aspectRatio( 0 )
    , m_headerAspectRatio( 0.0 )
    , m_size( QSizeF() )
    , m_wikipediaLabel( 0 )
    , m_webView( 0 )
    , m_current( "" )
    , m_reloadIcon( 0 )
    , m_albumIcon( 0 )
    , m_artistIcon( 0 )
    , m_lyricsIcon( 0 )
    , m_forwardIcon ( 0 )
    , m_backwardIcon ( 0 )
    , m_css( 0 )
    , m_albumState( false )
    , m_artistState( false )
    , m_lyricsState( false )
    , m_prevTrackInfoAvailable( false )
    , m_pageState( 0 )
    , m_wikiPreferredLang( QString() )
    , m_contextMenu( 0 )
{
    setHasConfigurationInterface( true );
    setBackgroundHints( Plasma::Applet::NoBackground );
}

WikiApplet::~ WikiApplet()
{
    delete m_webView;
    delete m_css;
}

void WikiApplet::init()
{
    m_wikipediaLabel = new QGraphicsSimpleTextItem( this );

    m_webView = new MyWebView( this );
    m_webView->setAttribute( Qt::WA_NoSystemBackground );

    // ask for all the CV height
    resize( 500, -1 );

    paletteChanged( App::instance()->palette() );
    connect( The::paletteHandler(), SIGNAL( newPalette( const QPalette& ) ), SLOT(  paletteChanged( const QPalette &  ) ) );

    m_webView->page()->setLinkDelegationPolicy ( QWebPage::DelegateAllLinks );
    connect( m_webView->page(), SIGNAL( linkClicked( const QUrl & ) ) , this, SLOT( linkClicked ( const QUrl & ) ) );

   // make transparent so we can use QPainter translucency to draw the  background
    QPalette palette = m_webView->palette();
    palette.setBrush(QPalette::Base, Qt::transparent);
    m_webView->page()->setPalette(palette);
    m_webView->setAttribute(Qt::WA_OpaquePaintEvent, false);


    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_wikipediaLabel->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    m_wikipediaLabel->setFont( labelFont );
    m_wikipediaLabel->setText( i18n( "Wiki-Info" ) );

    QAction* currentTrackAction = new QAction( i18n( "Current Track Info" ), this );
    currentTrackAction -> setIcon( KIcon( "go-home" ) );
    currentTrackAction -> setVisible( false );
    currentTrackAction -> setEnabled( false );
    m_currentTrackIcon = addAction( currentTrackAction );
    connect(currentTrackAction, SIGNAL( activated() ), this, SLOT( viewCurrentTrackInfo() ) );

    QAction* previousTrackAction = new QAction( i18n( "Previous Track Info" ), this );
    previousTrackAction -> setIcon( KIcon( "go-previous-page" ) );
    previousTrackAction -> setVisible( false );
    previousTrackAction -> setEnabled( false );
    m_previousTrackIcon = addAction( previousTrackAction );
    connect(m_previousTrackIcon,SIGNAL( activated() ), this, SLOT( viewPreviousTrackInfo() ) );

    QAction* backwardAction = new QAction( i18n( "Previous" ), this );
    backwardAction->setIcon( KIcon( "go-previous" ) );
    backwardAction->setVisible( true );
    backwardAction->setEnabled( false );
    m_backwardIcon = addAction( backwardAction );
    connect( backwardAction, SIGNAL( activated() ), this, SLOT( goBackward() ) );

    QAction* forwardAction = new QAction( i18n( "Next" ), this );
    forwardAction->setIcon( KIcon( "go-next" ) );
    forwardAction->setVisible( true );
    forwardAction->setEnabled( false );
    m_forwardIcon = addAction( forwardAction );
    connect( m_forwardIcon, SIGNAL( activated() ), this, SLOT( goForward() ) );

    QAction* reloadAction = new QAction( i18n( "Reload" ), this );
    reloadAction->setIcon( KIcon( "view-refresh" ) );
    reloadAction->setVisible( true );
    reloadAction->setEnabled( false );
    m_reloadIcon = addAction( reloadAction );
    connect( m_reloadIcon, SIGNAL( activated() ), this, SLOT( reloadWikipedia() ) );

    QAction* artistAction = new QAction( i18n( "Artist" ), this );
    artistAction->setIcon( KIcon( "filename-artist-amarok" ) );
    artistAction->setVisible( true );
    artistAction->setEnabled( false );
    m_artistIcon = addAction( artistAction );
    connect( m_artistIcon, SIGNAL( activated() ), this, SLOT( navigateToArtist() ) );

    QAction* albumAction = new QAction( i18n( "Album" ), this );
    albumAction->setIcon( KIcon( "filename-album-amarok" ) );
    albumAction->setVisible( true );
    albumAction->setEnabled( false );
    m_albumIcon = addAction( albumAction );
    connect( m_albumIcon, SIGNAL( activated() ), this, SLOT( navigateToAlbum() ) );

    QAction* lyricsAction = new QAction( i18n( "Lyrics" ), this );
    lyricsAction->setIcon( KIcon( "amarok_lyrics" ) );
    lyricsAction->setVisible( true );
    lyricsAction->setEnabled( false );
    m_lyricsIcon = addAction( lyricsAction );
    connect( m_lyricsIcon, SIGNAL( activated() ), this, SLOT( navigateToLyrics() ) );

    QAction* titleAction = new QAction( i18n( "Track" ), this );
    titleAction->setIcon( KIcon( "filename-title-amarok" ) );
    titleAction->setVisible( true );
    titleAction->setEnabled( false );
    m_titleIcon = addAction( titleAction );
    connect( m_titleIcon, SIGNAL( activated() ), this, SLOT( navigateToTitle() ) );


    QAction* langAction = new QAction( i18n( "Settings" ), this );
    langAction->setIcon( KIcon( "preferences-system" ) );
    langAction->setVisible( true );
    langAction->setEnabled( true );
    m_settingsIcon = addAction( langAction );
    connect( m_settingsIcon, SIGNAL( activated() ), this, SLOT( switchLang() ) );

    connectSource( "wikipedia-artist" );
    connectSource( "wikipedia-album" );
    connectSource( "wikipedia-lyrics" );
    connectSource( "wikipedia-web" );
    connectSource( "wikipedia-title" );

    connect( dataEngine( "amarok-wiki" ), SIGNAL( sourceAdded( const QString & ) ),
             this, SLOT( connectSource( const QString & ) ) );

    connect( dataEngine( "amarok-lyrics" ),SIGNAL( sourceAdded(const QString & ) ),this, SLOT( connectSource( const QString & ) ) );

    constraintsEvent();
    createContextMenu();
}

void WikiApplet :: createContextMenu()
{
     if(m_contextMenu != 0)
     {    delete m_contextMenu;
          m_contextMenu = 0;
     }
     m_contextMenu = new QMenu;

     QAction *previousTrackAction = m_contextMenu -> addAction( i18n( "View Previous Track Info" ) );
     previousTrackAction -> setIcon( KIcon( "go-previous-page" ) );
     previousTrackAction -> setVisible( false );
     previousTrackAction -> setEnabled( false );
     connect(previousTrackAction,SIGNAL(triggered()),this,SLOT( viewPreviousTrackInfo() ) );

     QAction *currentTrackAction = m_contextMenu -> addAction( i18n( "View Current Track Info" ) );
     currentTrackAction -> setIcon( KIcon( "go-home" ) );
     currentTrackAction -> setVisible( false );
     currentTrackAction -> setEnabled( false );
     connect(currentTrackAction,SIGNAL(triggered()),this,SLOT( viewCurrentTrackInfo() ) );

     QAction* refreshAction = m_contextMenu-> addAction( i18n( "Refresh Previous Track Info" ) );
     refreshAction->setIcon( KIcon( "view-refresh" ) );
     refreshAction->setVisible( false );
     refreshAction->setEnabled( false );
     connect( refreshAction , SIGNAL( triggered() ), this, SLOT( viewPreviousTrackInfo() ) );

     QAction *reloadLyricsAction = m_contextMenu -> addAction(i18n("Reload Lyrics Info"));
     reloadLyricsAction -> setIcon( KIcon( "view-refresh" ) );
     reloadLyricsAction -> setVisible( true );
     reloadLyricsAction -> setEnabled( false );
     connect(reloadLyricsAction,SIGNAL(triggered()),this,SLOT( reloadLyricsInfo() ) );

     QAction *reloadArtistAction = m_contextMenu -> addAction(i18n("Reload Artist Info"));
     reloadArtistAction -> setIcon( KIcon( "view-refresh" ) );
     reloadArtistAction -> setVisible( true );
     reloadArtistAction -> setEnabled( false );
     connect(reloadArtistAction,SIGNAL(triggered()),this,SLOT( reloadArtistInfo() ) );

     QAction *reloadAlbumAction = m_contextMenu -> addAction(i18n("Reload Album Info"));
     reloadAlbumAction -> setIcon( KIcon( "view-refresh" ) );
     reloadAlbumAction -> setVisible( true );
     reloadAlbumAction -> setEnabled( false );
     connect(reloadAlbumAction,SIGNAL(triggered()),this,SLOT( reloadAlbumInfo() ) );

     QAction *reloadTitleAction = m_contextMenu -> addAction(i18n("Reload Title Info"));
     reloadTitleAction -> setIcon( KIcon( "view-refresh" ) );
     reloadTitleAction -> setVisible( true );
     reloadTitleAction -> setEnabled( false );
     connect(reloadTitleAction,SIGNAL(triggered()),this,SLOT( reloadTitleInfo() ) );

     QAction *navigateLyricsAction = m_contextMenu -> addAction(i18n("See Lyrics Info") );
     navigateLyricsAction -> setVisible(false);
     connect(navigateLyricsAction,SIGNAL(triggered()),this,SLOT( navigateToLyrics() ) );

     QAction *navigateArtistAction = m_contextMenu -> addAction(i18n("See Artist Info") );
     navigateArtistAction -> setVisible(false);
     connect(navigateArtistAction,SIGNAL(triggered()),this,SLOT( navigateToArtist() ) );

     QAction *navigateAlbumAction = m_contextMenu -> addAction(i18n("See Album Info") );
     navigateAlbumAction -> setVisible(false);
     connect(navigateAlbumAction,SIGNAL(triggered()),this,SLOT( navigateToAlbum() ) );

     QAction *navigateTitleAction = m_contextMenu -> addAction(i18n("See Title Info") );
     navigateTitleAction -> setVisible(false);
     connect(navigateTitleAction,SIGNAL(triggered()),this,SLOT( navigateToTitle() ) );

     QAction *compressArtistAction = m_contextMenu -> addAction(i18n("Compress Artist Info") );
     compressArtistAction -> setVisible(false);
     connect(compressArtistAction,SIGNAL(triggered()),this,SLOT(compressArtistInfo() ) );

     QAction *compressLyricsAction = m_contextMenu -> addAction(i18n("Compress Lyrics Info"));
     compressLyricsAction -> setVisible(false);
     connect(compressLyricsAction,SIGNAL(triggered()),this,SLOT( compressLyricsInfo()));

     QAction *compressAlbumAction = m_contextMenu -> addAction(i18n("Compress Album Info"));
     compressAlbumAction -> setVisible(false);
     connect(compressAlbumAction,SIGNAL(triggered()),this,SLOT( compressAlbumInfo()));

     QAction *compressTitleAction = m_contextMenu -> addAction(i18n("Compress Title Info"));
     compressTitleAction -> setVisible(false);
     connect(compressTitleAction,SIGNAL(triggered()),this,SLOT( compressTitleInfo()));

     QAction *expandLyricsAction = m_contextMenu -> addAction(i18n("View More Lyrics Info" ) );
     expandLyricsAction -> setVisible(false);
     connect(expandLyricsAction,SIGNAL(triggered()),this,SLOT( expandLyricsInfo() ) );

     QAction *expandArtistAction = m_contextMenu -> addAction(i18n("View More Artist Info"));
     expandArtistAction -> setVisible(false);
     connect(expandArtistAction,SIGNAL(triggered()),this,SLOT( expandArtistInfo() ) );

     QAction *expandAlbumAction = m_contextMenu -> addAction(i18n("View More Album Info" ) );
     expandAlbumAction -> setVisible(false);
     connect(expandAlbumAction,SIGNAL(triggered()),this,SLOT( expandAlbumInfo() ) );

     QAction *expandTitleAction = m_contextMenu -> addAction(i18n("View More Title Info" ) );
     expandTitleAction -> setVisible(false);
     connect(expandTitleAction,SIGNAL(triggered()),this,SLOT( expandTitleInfo() ) );

     m_webView -> loadMenu(m_contextMenu);
}

Plasma::IconWidget *
WikiApplet::addAction( QAction *action )
{
    DEBUG_BLOCK
    if ( !action ) {
        debug() << "ERROR!!! PASSED INVALID ACTION";
        return 0;
    }

    Plasma::IconWidget *tool = new Plasma::IconWidget( this );
    tool->setAction( action );
    tool->setText( "" );
    tool->setToolTip( action->text() );
    tool->setDrawBackground( false );
    tool->setOrientation( Qt::Horizontal );
    QSizeF iconSize = tool->sizeFromIconSize( 16 );
    tool->setMinimumSize( iconSize );
    tool->setMaximumSize( iconSize );
    tool->resize( iconSize );
    tool->setZValue( zValue() + 1 );

    return tool;
}

void
WikiApplet::connectSource( const QString &source )
{

    if( source  == "wikipedia-artist" )
        dataEngine( "amarok-wiki" ) -> connectSource( "wikipedia-artist",this );

    if( source == "wikipedia-album" )
          dataEngine( "amarok-wiki" ) -> connectSource( "wikipedia-album",this );

    if( source == "wikipedia-title" )
        dataEngine( "amarok-wiki" ) -> connectSource( "wikipedia-title",this );

    if( source == "wikipedia-lyrics" )
        dataEngine( "amarok-lyrics" ) -> connectSource( "lyrics",this );
    if( source == "wikipedia-web")
        dataEngine( "amarok-wiki" ) -> connectSource( "wikipedia-web",this );

}

void WikiApplet::constraintsEvent( Plasma::Constraints constraints )
{

    prepareGeometryChange();

    float textWidth = m_wikipediaLabel->boundingRect().width();
    float offsetX =  ( boundingRect().width() - textWidth ) / 2;

    m_wikipediaLabel->setPos( offsetX,  standardPadding() + 2 );

    m_webView->setPos( standardPadding(), m_wikipediaLabel->pos().y() + m_wikipediaLabel->boundingRect().height() + standardPadding() );
    m_webView->resize( boundingRect().width() - 2 * standardPadding(), boundingRect().height() - m_webView->pos().y() - standardPadding() );

    float iconWidth = m_reloadIcon->size().width();

    m_backwardIcon->setPos( size().width() - 8 * iconWidth - 6 * standardPadding(), standardPadding() );
    m_forwardIcon->setPos( size().width() - 7 * iconWidth - 6 * standardPadding(), standardPadding() );

    m_lyricsIcon->setPos( size().width() - 6 * iconWidth - 4 * standardPadding(), standardPadding() );

    m_artistIcon->setPos( size().width() - 5 * iconWidth - 4 * standardPadding(), standardPadding() );

    m_albumIcon->setPos( size().width() - 4 * iconWidth - 4 * standardPadding(), standardPadding() );
    m_previousTrackIcon->setPos( size().width() - 4 * iconWidth - 4 * standardPadding(), standardPadding() );

    m_titleIcon->setPos( size().width() - 3 * iconWidth - 4 * standardPadding(), standardPadding() );
     m_currentTrackIcon->setPos( size().width() - 3 * iconWidth - 4 * standardPadding(), standardPadding() );

    m_settingsIcon->setPos( size().width() - 2 * iconWidth - 2 * standardPadding(), standardPadding() );

    m_reloadIcon->setPos( size().width() - iconWidth - standardPadding(), standardPadding() );
}


void
WikiApplet::linkClicked( const QUrl &pageurl )
{
    DEBUG_BLOCK
    debug() << "URL: " << pageurl;
    QString m = pageurl.toString();

    if ( pageurl.toString().contains( "wikipedia.org/" ) )
    {
        dataEngine( "amarok-wiki" )->query( QString( "wikipedia:get:" ) + pageurl.toString() );
        m_histoFor.clear();
        if( m_forwardIcon->action() && m_forwardIcon->action()->isEnabled() )
            m_forwardIcon->action()->setEnabled( false );
        return;
    }

    if(m.contains("artist") )
    {

       m_artistState = m_artistState ?false : true;
       updateWebView();
       navigateToArtist();
       m_webView -> toggleAction("Artist",m_artistState);
       return;
    }
    else if( m.contains("album") )
    {
        m_albumState = m_albumState ? false : true;
        updateWebView();
        navigateToAlbum();
        m_webView -> toggleAction("Album",m_albumState);
        return;
    }
    else if( m.contains("title") )
    {
        m_titleState = m_titleState ? false : true;
        updateWebView();
        navigateToTitle();
        m_webView -> toggleAction("Title",m_titleState);
        return;
    }

    else if( m.contains("lyrics") )
    {
        m_lyricsState = m_lyricsState ? false : true;
        updateWebView();
        navigateToLyrics();
        m_webView -> toggleAction("Lyrics",m_lyricsState);
        return;
    }
    QDesktopServices::openUrl( pageurl.toString() );
}

void WikiApplet :: updateWebPageIcons()
{
    DEBUG_BLOCK
    if( m_lyricsIcon -> action() )
    {
        m_lyricsIcon -> action() -> setVisible( false );
        m_lyricsIcon -> action() -> setEnabled( false );
    }
    if( m_artistIcon -> action() )
    {
        m_artistIcon -> action() -> setVisible( false );
        m_artistIcon -> action() -> setEnabled( false );
    }
    if( m_albumIcon -> action() )
    {
        m_albumIcon -> action() -> setVisible( false );
        m_albumIcon -> action() -> setEnabled( false );
    }
    if( m_titleIcon -> action() )
    {
        m_titleIcon -> action() -> setVisible( false );
        m_titleIcon -> action() -> setVisible( false );
    }
    if( m_currentTrackIcon -> action() )
    {
        m_currentTrackIcon -> action() -> setVisible( true );
        m_currentTrackIcon -> action() -> setEnabled( true );
    }
    if( m_previousTrackIcon -> action() )
    {
        m_previousTrackIcon -> action() -> setVisible( true );
        m_previousTrackIcon -> action() -> setEnabled( false );
    }

    if( m_prevTrackInfoAvailable &&  m_previousTrackIcon -> action() && !m_previousTrackIcon -> isEnabled() )
        m_previousTrackIcon -> action() -> setEnabled( true );

    m_webView-> disableAllActions();

    m_pageState = 1;
}

void
WikiApplet :: viewCurrentTrackInfo()
{
    DEBUG_BLOCK
    if( m_lyricsIcon -> action() )
    {
        m_lyricsIcon -> action() -> setVisible( true );
        m_lyricsIcon -> action() -> setEnabled( true );
    }
    if( m_artistIcon -> action() )
    {
        m_artistIcon -> action() -> setVisible( true );
        m_artistIcon -> action() -> setEnabled( true );
    }
    if( m_albumIcon -> action() )
    {
        m_albumIcon -> action() -> setVisible( true );
        m_albumIcon -> action() -> setEnabled( true );
    }
    if( m_titleIcon -> action() )
    {
        m_titleIcon -> action() -> setVisible( true );
        m_titleIcon -> action() -> setVisible( true );
    }
    if( m_currentTrackIcon -> action() )
    {
        m_currentTrackIcon -> action() -> setVisible( false );
        m_currentTrackIcon -> action() -> setEnabled( false );
    }
    if( m_previousTrackIcon -> action() )
    {
        m_previousTrackIcon -> action() -> setVisible( false );
        m_previousTrackIcon -> action() -> setEnabled( false );
    }

    if( m_prevTrackInfoAvailable )
    {    m_webView -> enablePreviousTrackAction(true);
         m_webView -> enableCurrentTrackAction( false) ;
    }

     if( m_pageState == 1 && !m_current.isEmpty() )
     {
         m_histoBack.push_front(m_current);
        if ( !m_histoBack.empty() && m_backwardIcon->action() && !m_backwardIcon->action()->isEnabled() )
                    m_backwardIcon->action()->setEnabled( true );
     }
    m_pageState = 0;

    resetStates();
    reloadLyricsInfo();
    reloadArtistInfo();
    reloadAlbumInfo();
    reloadTitleInfo();
}

void
WikiApplet :: viewPreviousTrackInfo()
{
    DEBUG_BLOCK
    if( m_lyricsIcon -> action() )
    {
        m_lyricsIcon -> action() -> setVisible( true );
        m_lyricsIcon -> action() -> setEnabled( true );
    }
    if( m_artistIcon -> action() )
    {
        m_artistIcon -> action() -> setVisible( true );
        m_artistIcon -> action() -> setEnabled( true );
    }
    if( m_albumIcon -> action() )
    {
        m_albumIcon -> action() -> setVisible( true );
        m_albumIcon -> action() -> setEnabled( true );
    }
    if( m_titleIcon -> action() )
    {
        m_titleIcon -> action() -> setVisible( true );
        m_titleIcon -> action() -> setVisible( true );
    }
    if( m_currentTrackIcon -> action() )
    {
        m_currentTrackIcon -> action() -> setVisible( false );
        m_currentTrackIcon -> action() -> setEnabled( false );
    }
    if( m_previousTrackIcon -> action() )
    {
        m_previousTrackIcon -> action() -> setVisible( false );
        m_previousTrackIcon -> action() -> setEnabled( false );
    }
    if( m_pageState == 1 && !m_current.isEmpty() )
    {
         m_histoBack.push_front(m_current);
        if ( !m_histoBack.empty() && m_backwardIcon->action() && !m_backwardIcon->action()->isEnabled() )
                    m_backwardIcon->action()->setEnabled( true );
    }

    m_webView -> enableCurrentTrackAction(true );
    m_webView -> enablePreviousTrackAction( false );

    m_pageState = -1;
    resetStates();
    dataEngine( "amarok-wiki" )-> query( "wikipedia:previous track info" );
    dataEngine( "amarok-lyrics" )-> query( "previous lyrics" );
}

void
WikiApplet :: reloadArtistInfo()
{
    dataEngine( "amarok-wiki" )->query( "wikipedia:reload:artist" );
}

void
WikiApplet :: reloadAlbumInfo()
{
    dataEngine( "amarok-wiki" )->query( "wikipedia:reload:album" );
}

void
WikiApplet :: reloadLyricsInfo()
{
    Meta::TrackPtr curtrack = The::engineController()->currentTrack();

    if( !curtrack || !curtrack->artist() )
        return;

    ScriptManager::instance()->notifyFetchLyrics( curtrack->artist()->name(), curtrack->name() );
}

void
WikiApplet :: reloadTitleInfo()
{
    dataEngine( "amarok-wiki" )->query( "wikipedia:reload:title" );
}

void
WikiApplet :: navigateToArtist()
{
     QString command = "window.location.href=\"#artist\"";
     m_webView -> mainFrame() -> evaluateJavaScript(command);
}

void
WikiApplet :: navigateToAlbum()
{
    QString command = "window.location.href=\"#album\"";
    m_webView -> mainFrame() -> evaluateJavaScript(command);
}

void
WikiApplet :: navigateToLyrics()
{
    QString command = "window.location.href=\"#lyrics\"";
    m_webView -> mainFrame() -> evaluateJavaScript(command);
}

void
WikiApplet :: navigateToTitle()
{
    QString command = "window.location.href=\"#title\"";
    m_webView -> mainFrame() -> evaluateJavaScript(command);
}

void
WikiApplet :: compressArtistInfo()
{
    m_artistState = true;
    linkClicked( QUrl("#artist") ) ;
}

void
WikiApplet :: compressAlbumInfo()
{
    m_albumState = true;
    linkClicked( QUrl("#album") ) ;
}

void
WikiApplet :: compressTitleInfo()
{
    m_titleState = true;
    linkClicked( QUrl("#title") ) ;
}

void
WikiApplet :: compressLyricsInfo()
{
    m_lyricsState = true;
    linkClicked( QUrl("#lyrics") ) ;
}


void
WikiApplet :: expandArtistInfo()
{
    m_artistState = false;
    linkClicked(  QUrl("#artist") ) ;
}

void
WikiApplet :: expandAlbumInfo()
{
    m_albumState = false;
    linkClicked( QUrl("#album") );
}

void
WikiApplet :: expandLyricsInfo()
{
    m_lyricsState = false;
    linkClicked( QUrl("#lyrics") );
}

void
WikiApplet :: expandTitleInfo()
{
    m_titleState = false;
    linkClicked( QUrl("#title") );
}

void
WikiApplet :: goForward()
{
    DEBUG_BLOCK

    if( !m_histoFor.empty() )
    {

        if( m_pageState == 1 )
            m_histoBack.push_front( m_current );
        m_current = m_histoFor.front();
        m_histoFor.pop_front();
        m_webView->setHtml( m_current , KUrl( QString() ) );

        if(m_pageState == 1 &&  m_backwardIcon->action() && !m_backwardIcon->action()->isEnabled() )
            m_backwardIcon->action()->setEnabled( true );

        if ( m_histoFor.empty() && m_forwardIcon->action()->isEnabled() )
            m_forwardIcon->action()->setEnabled( false );

        if(!m_pageState || m_pageState == -1)
            updateWebPageIcons();

    }
}

void
WikiApplet :: goBackward()
{
     DEBUG_BLOCK
    if( !m_histoBack.empty() )
    {

        if( m_pageState == 1)
            m_histoFor.push_front( m_current );
        m_current =  m_histoBack.front();
        m_histoBack.pop_front();
        m_webView->setHtml( m_current , KUrl( QString() ) );
        if( m_pageState == 1 && m_forwardIcon->action() && !m_forwardIcon->action()->isEnabled() )
            m_forwardIcon->action()->setEnabled( true );

        if ( m_histoBack.empty() && m_backwardIcon->action()->isEnabled() )
            m_backwardIcon->action()->setEnabled( false );

        if(!m_pageState || m_pageState == -1)
            updateWebPageIcons();

    }
}

void
WikiApplet :: enablePrevTrackIcon()
{
    DEBUG_BLOCK

    if( !m_previousTrackIcon -> action() -> isEnabled() && ( m_pageState == 1 || m_pageState == -1 ))
    {
        m_previousTrackIcon -> action() -> setVisible( true );
        m_previousTrackIcon -> action() -> setEnabled( true );
    }
    else if( m_pageState == -1 )
    {
        resetStates();
        dataEngine( "amarok-wiki" )-> query( "wikipedia:previous track info" );
        dataEngine( "amarok-lyrics" )-> query( "previous lyrics" );
    }

}

void
WikiApplet :: switchLang()
{
    DEBUG_BLOCK
    showConfigurationInterface();
}

void
WikiApplet :: switchToLang(QString lang)
{
    DEBUG_BLOCK
    // TODO change this b/c it's BAAADDD !!!
    if (lang == i18nc("automatic language selection", "Automatic") )
        m_wikiPreferredLang = "aut";

    else if (lang == i18n("English") )
        m_wikiPreferredLang = "en";

    else if (lang == i18n("French") )
        m_wikiPreferredLang = "fr";

    else if (lang == i18n("German") )
        m_wikiPreferredLang = "de";

    dataEngine( "amarok-wiki" )->query( QString( "wikipedia:lang:" ) + m_wikiPreferredLang );

    KConfigGroup config = Amarok::config("Wiki Applet");
    config.writeEntry( "PreferredLang", m_wikiPreferredLang );
    dataEngine( "amarok-wiki" )->query( QString( "wikipedia:lang:" ) + m_wikiPreferredLang );
}

void
WikiApplet::createConfigurationInterface( KConfigDialog *parent )
{
    DEBUG_BLOCK
    KConfigGroup configuration = config();
    QWidget *settings = new QWidget;
    ui_Settings.setupUi( settings );

    // TODO bad, it's done manually ...
    if ( m_wikiPreferredLang == "aut" )
        ui_Settings.comboBox->setCurrentIndex( 0 );
    else if ( m_wikiPreferredLang == "en" )
        ui_Settings.comboBox->setCurrentIndex( 1 );
    else if ( m_wikiPreferredLang == "fr" )
        ui_Settings.comboBox->setCurrentIndex( 2 );
    else if ( m_wikiPreferredLang == "de" )
        ui_Settings.comboBox->setCurrentIndex( 3 );

    parent->addPage( settings, i18n( "Wiki Settings" ), "preferences-system");
    connect( ui_Settings.comboBox, SIGNAL( currentIndexChanged( QString ) ), this, SLOT( switchToLang( QString ) ) );
}

bool WikiApplet::hasHeightForWidth() const
{
    return true;
}

qreal WikiApplet::heightForWidth( qreal width ) const
{
    return width * m_aspectRatio;
}

void WikiApplet :: resetStates()
{
    m_albumState = false;
    m_lyricsState = false;
    m_artistState = false;
    m_titleState = false;
}

void WikiApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data ) // SLOT
{
        DEBUG_BLOCK
        debug() << "The Name of the Source is : "<< name;
        if( data.size() == 0 ) return;

        //inform the applet about the track change
        if( data.contains( "meta" ) )
        {
                m_prevTrackInfoAvailable = true;
                if( m_pageState == 1)
                    enablePrevTrackIcon();
                return;
        }

         //if the previous track page is being viewed and update about current
        //page is received then we ignore it to avoid overwriting of web view
        if( m_pageState == -1 )
        {
            if( !( name == "wikipedia-web" || ( data.contains( "label" ) && data["label"].toString().contains( "previous" ) ) ) )
                    return;
        }
        if( name == "wikipedia-web" )
        {

            if( data.contains( "page" ) )
            {
                if(m_pageState == 1)
                {
                    m_histoBack.push_front( m_current );
                    while ( m_histoBack.size() > 20 )
                        m_histoBack.pop_back();
                     if ( m_backwardIcon->action() && !m_backwardIcon->action()->isEnabled() )
                    m_backwardIcon->action()->setEnabled( true );
                }

                m_webView -> setHtml( data[ "page"].toString());
                //since you're moving out of your home page displaying track Details
                //set pageState to true
                if(!m_pageState || m_pageState == -1)
                    updateWebPageIcons();
                m_current = data["page"].toString();
                m_histoFor.clear();
                if ( m_forwardIcon->action() && m_forwardIcon->action()->isEnabled() )
                    m_forwardIcon->action()->setEnabled( false );

            }
            else if( data.contains( "message" ) )
                m_webView -> setHtml( data["message"].toString());
            return;
        }


        if(!m_pageState || m_pageState == -1)
        {
            if( name.contains( "artist" ) )
                updateArtistInfo(data);
            if(name .contains( "album" ) )
                updateAlbumInfo(data);
            if(name.contains( "lyrics" ) )
                updateLyricsInfo(data);
            if(name.contains( "title" ) )
                updateTitleInfo( data );

            if( m_pageState == -1)
            {
                m_webView -> disableReloadActions( "Album" );
                m_webView -> disableReloadActions( "Artist" );
                m_webView -> disableReloadActions( "Lyrics" );
                m_webView -> disableReloadActions( "Title" );
            }

            //update only when the current page is the home page or previous track's page

            updateWebView();
        }

        if( m_prevTrackInfoAvailable  && !m_pageState )
            m_webView -> enablePreviousTrackAction( true );

        if( m_reloadIcon->action() && !m_reloadIcon->action()->isEnabled() )
        {
            m_reloadIcon->action()->setEnabled( true );
            //for some reason when we enable the action suddenly the icon has the text "..."
            m_reloadIcon->action()->setText( "" );
        }
         if( m_artistIcon->action() && !m_artistIcon->action()->isEnabled() )
            m_artistIcon->action()->setEnabled( true );

        if( m_albumIcon->action() && !m_albumIcon->action()->isEnabled() )
            m_albumIcon->action()->setEnabled( true );

        if( m_lyricsIcon->action() && !m_lyricsIcon->action()->isEnabled() )
            m_lyricsIcon->action()->setEnabled( true );

        if( m_titleIcon->action() && !m_titleIcon->action()->isEnabled() )
            m_titleIcon->action()->setEnabled( true );

}


void WikiApplet :: updateLyricsInfo( const Plasma :: DataEngine :: Data& data )
{

        QTextDocument w;
        int flag = 0;

        m_lyricsHtml = m_compressedLyricsHtml =  "<h3 id=\"lyrics\"> Lyrics</h3>";
        if( data.contains( "noscriptrunning" ) )
        {
           w.setPlainText(( i18n( "No lyrics script is running." ) ) );
           flag = 1;
        }
        else if( data.contains( "fetching" ) )
        {
            w.setPlainText( i18n( "Lyrics are being fetched..." ) );
            flag = 1;
        }
        else if( data.contains( "error" ) )
        {
            w.setPlainText( i18n( "Lyrics were not able to be downloaded. Please check your internet connection: %1", data["error"].toString() ) );
            flag = 1;
        }
        else if( data.contains( "suggested" ) )
        {
            QVariantList suggested = data[ "suggested" ].toList();
            // build simple HTML to show
            // a list
            QString html = QString( "<br><br>" );
            foreach( const QVariant &suggestion, suggested )
            {
                    QString sug = suggestion.toString();
                    //parsing Suggestion
                    QStringList pieces = sug.split( " - " );
                    QString link = QString( "<a href=\"%1|%2|%3\">%4 - %5</a><br>" ).arg( pieces[ 0 ] ).arg( pieces[ 1 ] ).arg( pieces[ 2 ] ).arg( pieces[ 1 ] ).arg( pieces[ 0 ] );
                    html += link;
            }
            w.setHtml(html);
            debug() << "setting html: " << html;
            m_webView -> enableNavigateAction("Lyrics");
            flag = 1;

        }
        else if( data.contains( "html" ) )
        {
            // show pure html in the text area
            w.setPlainText( data[ "html" ].toString() );
            debug() <<" PLAIN TEXT " << w.toPlainText();
        }
        else if( data.contains( "lyrics" ) )
        {
            QVariantList lyrics  = data[ "lyrics" ].toList();
            m_lyricsTitle = QString( " %1 : %2 - %3" ).arg( i18n( "Lyrics" ) ).arg( lyrics[ 0 ].toString() ).arg( lyrics[ 1 ].toString() );
            //  need padding for title
            w.setPlainText( lyrics[ 3 ].toString().trimmed() );
        }
        else if( data.contains( "notfound" ) || data.contains( "Unavailable" ) )
        {
            w.setPlainText( i18n( "Lyrics Unavailable for this track" ) );
            flag = 1;
        }
        m_lyricsHtml += w.toHtml();

        if(flag)
        {    if( !m_pageState )
                m_webView -> setDefaultActions("Lyrics");

             m_webView -> disableViewActions( "Lyrics" );
             m_compressedLyricsHtml = m_lyricsHtml;
             m_lyricsState = false;
        }
        else
        {

             int count = m_lyricsHtml.count("<p");
             debug() <<"COUNT" << count <<"STRING CHECK : " << m_lyricsHtml;
             if(count >= 6)
             {
                int start = m_lyricsHtml.indexOf("<p",0);
                int i = 0,pos = start+3,end;
                while(i < 6)
                {
                    end = m_lyricsHtml.indexOf("</p>",pos);
                    i++;
                    pos = end + 3;
                }
                m_compressedLyricsHtml += m_lyricsHtml.mid(start,pos-start+1);
                m_lyricsHtml += "<p><a href= \"http:\\\\www.wikilyrics.amarok\">View Less...</a></p>";
                m_compressedLyricsHtml += "<p><a href= \"http:\\\\www.wikilyrics.amarok\">View More...</a></p>";
                m_lyricsState = false;
                m_webView -> enableViewActions(QString("Lyrics") );
                m_webView -> enableNavigateAction(QString("Lyrics"));
              }
            else
            {    if( !m_pageState )
                    m_webView -> setDefaultActions( "Lyrics" );
                 m_webView -> disableViewActions( "Lyrics" );
                 m_compressedLyricsHtml = m_lyricsHtml;
                 m_lyricsState = false;
            }
        }

        setPreferredSize( (int)size().width(), (int)size().height() );
        updateConstraints();
        update();

}

void WikiApplet::updateArtistInfo( const Plasma :: DataEngine :: Data& data )
{
    DEBUG_BLOCK

    if( data.contains( "title" ) )
        m_artistTitle = data[ "title" ].toString();

    m_artistHtml = "<h3 id=\"artist\">";
    m_artistHtml += m_artistTitle;
    m_artistHtml += "</h3>";
    m_compressedArtistHtml = m_artistHtml;

    if( data.contains( "page" ) )
    {
            m_artistHtml += data[ "page" ].toString();
            m_artistHtml += "<p><a href=\"http:\\\\www.wikiartist.amarok\">View Less...</a></p>";
            int startIndex = m_artistHtml.indexOf("<p>");
            int endIndex = m_artistHtml.indexOf("</p>")+3;
            m_compressedArtistHtml += "<html><body>";
            m_compressedArtistHtml += m_artistHtml.mid(startIndex,endIndex-startIndex+1);
            m_compressedArtistHtml += "<p><a href= \"http:\\\\www.wikiartist.amarok\">View More...</a></p>";
            m_compressedArtistHtml += "</body></html>";
            m_webView -> enableViewActions(QString("Artist"));
            m_webView -> enableNavigateAction( QString( "Artist" ) );
            m_artistState = false;
    }
    else if(data.contains( "message" ) )
    {
          m_artistHtml = m_compressedArtistHtml += data[ "message" ] .toString();
          if(!m_pageState)
                m_webView -> setDefaultActions("Artist");
          m_webView -> disableViewActions( "Artist" );
          m_artistState = false;
	}

    if( data.contains( "label" ) )
        m_label = data[ "label" ].toString() + ':';

    else
        m_label.clear();

}

void WikiApplet :: updateAlbumInfo(const Plasma :: DataEngine :: Data& data )
{
    DEBUG_BLOCK

    if( data.contains( "title" ) )
        m_albumTitle = data[ "title" ].toString();

     m_albumHtml = "<h3 id = \"album\">";
     m_albumHtml += m_albumTitle;
     m_albumHtml += "</h3>";
     m_compressedAlbumHtml = m_albumHtml;

     if( data.contains( "page" ) )
     {
            m_albumHtml += data[ "page" ].toString();
            m_albumHtml += "<p><a href = \"http:\\\\www.wikialbum.amarok\">View Less...</a></p>";
            int startIndex = m_albumHtml.indexOf("<p>");
            int endIndex = m_albumHtml.indexOf("</p>")+3;
            m_compressedAlbumHtml += m_albumHtml.mid(startIndex,endIndex-startIndex+1);
            m_compressedAlbumHtml += "<p><a href= \"http:\\\\www.wikialbum.amarok\">View More...</a></p>";
            m_webView -> enableViewActions( QString( "Album" ) );
            m_webView -> enableNavigateAction( QString( "Album") );
            m_albumState = false;
    }
    else if( data.contains( "message" ))
    {     m_compressedAlbumHtml += data["message"].toString();
          m_albumState = false;
          if(!m_pageState)
                m_webView -> setDefaultActions("Album");
          m_webView -> disableViewActions( "Album" );
    }
    if( data.contains( "label" ) )
    {    m_label = data[ "label" ].toString() + ':';
    }
    else
        m_label.clear();
}

void WikiApplet :: updateTitleInfo(const Plasma :: DataEngine :: Data& data )
{
    DEBUG_BLOCK

    if( data.contains( "title" ) )
        m_trackTitle = data[ "title" ].toString();

     m_titleHtml = "<h3 id = \"title\">";
     m_titleHtml += m_trackTitle;
     m_titleHtml += "</h3>";
     m_compressedTitleHtml = m_titleHtml;

     if( data.contains( "page" ) )
     {
            m_titleHtml += data[ "page" ].toString();
            m_titleHtml += "<p><a href = \"http:\\\\www.wikititle.amarok\">View Less...</a></p>";
            int startIndex = m_titleHtml.indexOf("<p>");
            int endIndex = m_titleHtml.indexOf("</p>")+3;
            m_compressedTitleHtml += m_titleHtml.mid(startIndex,endIndex-startIndex+1);
            m_compressedTitleHtml += "<p><a href= \"http:\\\\www.wikititle.amarok\">View More...</a></p>";
            m_webView -> enableViewActions( QString( "Title" ) );
            m_webView -> enableNavigateAction( QString( "Title") );
            m_titleState = false;
    }
    else if( data.contains( "message" ))
    {     m_compressedTitleHtml += data["message"].toString();
          m_titleState = false;
          if(!m_pageState)
                m_webView -> setDefaultActions("Title");
          m_webView -> disableViewActions( "Title" );
    }
    if( data.contains( "label" ) )
    {    m_label = data[ "label" ].toString() + ':';
    }
    else
        m_label.clear();
}


void WikiApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option )
    Q_UNUSED( contentsRect )
    p->setRenderHint( QPainter::Antialiasing );

    addGradientToAppletBackground( p );

    // draw rounded rect around title
    drawRoundedRectAroundText( p, m_wikipediaLabel );


    //draw background of wiki text
    p->save();
    QColor bg( App::instance()->palette().highlight().color() );
    bg.setHsvF( bg.hueF(), 0.07, 1, bg.alphaF() );
    QRectF wikiRect = m_webView->boundingRect();
    wikiRect.moveTopLeft( m_webView->pos() );
    QPainterPath round;
    round.addRoundedRect( wikiRect, 3, 3 );
    p->fillPath( round , bg  );
    p->restore();

}

void
WikiApplet::reloadWikipedia()
{
    DEBUG_BLOCK
   reloadArtistInfo();
   reloadAlbumInfo();
   reloadLyricsInfo();
   reloadTitleInfo();

}

void
WikiApplet::paletteChanged( const QPalette & palette )
{

  //  m_webView->setStyleSheet( QString( "QTextBrowser { background-color: %1; border-width: 0px; border-radius: 0px; color: %2; }" ).arg( Amarok::highlightColor().lighter( 150 ).name() )
  //                                                                                                            .arg( Amarok::highlightColor().darker( 400 ).name() ) );
    //m_webView->page()->settings()->setUserStyleSheetUrl( "file://" + KStandardDirs::locate("data", "amarok/data/WikipediaCustomStyle.css" ) );
    // read css, replace color placeholders, write to file, load into page
    QFile file( KStandardDirs::locate("data", "amarok/data/WikiCustomStyle.css" ) );
    if( file.open(QIODevice::ReadOnly | QIODevice::Text) )
    {
        QColor highlight( App::instance()->palette().highlight().color() );
        highlight.setHsvF( highlight.hueF(), 0.07, 1, highlight.alphaF() );

        QString contents = QString( file.readAll() );
        //debug() << "setting background:" << Amarok::highlightColor().lighter( 130 ).name();
        contents.replace( "{background_color}", PaletteHandler::highlightColor( 0.12, 1 ).name() );
        contents.replace( "{text_background_color}", highlight.name() );
        contents.replace( "{border_color}", highlight.name() );
        contents.replace( "{text_color}", palette.brush( QPalette::Text ).color().name() );
        contents.replace( "{link_color}", palette.link().color().name() );
        contents.replace( "{link_hover_color}", palette.link().color().darker( 200 ).name() );
        highlight.setHsvF( highlight.hueF(), 0.3, .95, highlight.alphaF() );
        contents.replace( "{shaded_text_background_color}", highlight.name() );
        contents.replace( "{table_background_color}", highlight.name() );
        contents.replace( "{headings_background_color}", highlight.name() );

        QColor t = PaletteHandler::highlightColor( 0.4, 1.05 );
        t.setAlpha( 120 );
        contents.replace( "{seperator_color}",t.name() );

        delete m_css;
        m_css = new KTemporaryFile();
        m_css->setSuffix( ".css" );
        if( m_css->open() )
        {
            m_css->write( contents.toLatin1() );

            QString filename = m_css->fileName();
            m_css->close(); // flush buffer to disk
            debug() << "set user stylesheet to:" << "file://" + filename;
            m_webView->page()->settings()->setUserStyleSheetUrl( "file://" + filename );
        }
    }
}

void
WikiApplet :: updateWebView()
{
        DEBUG_BLOCK
        QString display;

       display = m_lyricsState ? m_lyricsHtml : m_compressedLyricsHtml;

       display += "<hr/>";

       display += m_artistState ? m_artistHtml : m_compressedArtistHtml;

       display += "<hr/>";

       display += m_albumState ? m_albumHtml : m_compressedAlbumHtml;

       display += "<hr/>";

       display += m_titleState ? m_titleHtml : m_compressedTitleHtml;

       m_webView -> setHtml(display);
}

// My Web View class Definition begins here

MyWebView :: MyWebView ( QGraphicsItem *parent ) : Plasma :: WebView(parent)
{
    m_contextMenu = 0;
    m_hideMenu = false;
}

MyWebView :: ~MyWebView ()
{
    if(m_contextMenu)
      delete m_contextMenu;
    m_contextMenu = 0;
}


void MyWebView :: loadMenu( QMenu *menu)
{
    if( menu == 0)
        return;
    m_contextMenu = menu;

}

void MyWebView :: contextMenuEvent( QGraphicsSceneContextMenuEvent *event )
{
         if (!page()) {
         QGraphicsWidget::contextMenuEvent(event);
         return;
     }
     if( m_hideMenu )
     {    WebView ::contextMenuEvent( event );
          return;
     }

      m_contextMenu -> exec(event -> screenPos());
}

void MyWebView :: disableAllActions()
{
        QList<QAction*> actionList = m_contextMenu -> actions();
        foreach( QAction *action , actionList )
        {
            action -> setEnabled( false );
            action -> setVisible( false );

        }
        m_hideMenu = true;
}

void MyWebView :: enableCurrentTrackAction(bool state)
{
        if( state )
            m_hideMenu = false;

        QList<QAction*> actionList = m_contextMenu -> actions();
        foreach( QAction *action , actionList )
            if( action->text().contains( "Current" ) || action -> text().contains( "Refresh" ) )
            {
                action -> setEnabled( state );
                action -> setVisible( state );
            }

}

void MyWebView :: enablePreviousTrackAction(bool state )
{
        m_hideMenu = false;
        QList<QAction*> actionList = m_contextMenu -> actions();
        foreach( QAction *action , actionList )
        {
            if( action->text().contains( "Previous" ) && !action->text().contains("Refresh") )
            {
                action -> setEnabled( state );
                action -> setVisible( state );
            }
        }
}



void MyWebView :: toggleAction(const QString s,bool status )
{
    QList<QAction*> actionList = m_contextMenu -> actions();
    if( !(s == "Artist" || s == "Album" || s == "Lyrics" || s == "Title" ) )
      return;

    m_hideMenu = false;
    if(status)
    {
        foreach(QAction *action,actionList)
        {
            if( action -> text().contains(s)&& action->text().contains("Compress") )
            {    action -> setVisible(true);
                 action -> setEnabled( true );
            }
            if( action -> text().contains(s) && action -> text().contains("More" ) )
            {    action -> setVisible(false);
                 action -> setEnabled( false );
            }
        }
    }
    else
    {
        foreach(QAction *action,actionList)
        {
            if( action -> text().contains(s)&& action->text().contains("Compress") )
            {    action -> setVisible(false);
                 action -> setEnabled( false );
            }
            if( action -> text().contains(s) && action -> text().contains("More" ) )
            {    action -> setVisible(true);
                 action -> setEnabled( true );
            }
        }
    }



}

void MyWebView :: enableViewActions(const QString s)
{
    QList<QAction*> actionList = m_contextMenu -> actions();
     if( !(s == "Artist" || s == "Album" || s == "Lyrics" || s == "Title" ) )
      return;

     m_hideMenu = false;

        foreach(QAction *action,actionList)
        {
            if( action -> text().contains(s)&& action->text().contains("Compress") )
            {    action -> setVisible(false);
                 action -> setEnabled( false );
            }
            else if(action -> text().contains(s))
            {    action -> setVisible(true);
                 action -> setEnabled( true );
            }
        }

}

void MyWebView :: disableViewActions( const QString s )
{
    QList<QAction*> actionList = m_contextMenu -> actions();
     if( !(s == "Artist" || s == "Album" || s == "Lyrics"|| s == "Title" ) )
      return;
     m_hideMenu = false;

        foreach(QAction *action,actionList)
        {
            if( action -> text().contains(s)&& ( action->text().contains("Compress") || action -> text().contains("More" )  || action -> text().contains( "See" ) ) )
            {    action -> setVisible(false);
                 action -> setEnabled( false );
            }
        }
}

void MyWebView :: setDefaultActions(const QString s)
{
      QList<QAction*> actionList = m_contextMenu -> actions();
      if( !(s == "Artist" || s == "Album" || s == "Lyrics" || s == "Title") )
      return;
      m_hideMenu = false;

        foreach(QAction *action,actionList)
        {
            if( action -> text().contains(s)&& action->text().contains("Reload") )
            {    action -> setVisible(true);
                 if(!action -> isEnabled())
                     action->setEnabled( true );
            }
            else if(action -> text().contains(s))
                action -> setVisible(false);
        }

}

void MyWebView :: disableReloadActions( const QString s )
{
    QList<QAction*> actionList = m_contextMenu -> actions();
      if( !(s == "Artist" || s == "Album" || s == "Lyrics" || s == "Title") )
      return;

        foreach(QAction *action,actionList)
            if( action -> text().contains(s) && action->text().contains("Reload") )
            {
                 action -> setVisible(false);
                 action->setEnabled( false );
            }
}


void MyWebView :: enableNavigateAction(const QString s)
{
     QList<QAction*> actionList = m_contextMenu -> actions();
     if( !(s == "Artist" || s == "Album" || s == "Lyrics" || s == "Title" ) )
      return;
    m_hideMenu = false;
     foreach(QAction *action,actionList)
     {
          if( action -> text().contains(s)&& action->text().contains("See") )
          {      action -> setVisible(true);
                 action -> setEnabled( true );
          }
     }
}

#include "WikiApplet.moc"


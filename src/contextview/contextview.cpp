/***************************************************************************
 * copyright     : (C) 2007 Seb Ruiz <ruiz@kde.org>                        *
 *                 (C) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com> *
*                  (C) 2007 Leonardo Franchi <lfranchi@gmail.com>          *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "amarok.h" //oldForeach
#include "amarokconfig.h"
#include "debug.h"
#include "albumbox.h"
#include "cloudbox.h"
#include "GenericInfoBox.h"
#include "textfader.h"
#include "collectiondb.h"
#include "contextbox.h"
#include "contextview.h"
#include "debug.h"
#include "enginecontroller.h"
#include "graphicsitemfader.h"
#include "introanimation.h"
#include "scriptmanager.h"
#include "statusbar.h"

#include <kstandarddirs.h>

#include <math.h> // scaleView()
#include <qdom.h>
#include <QBrush>
#include <QColor>
#include <QGraphicsTextItem>
#include <QGraphicsScene>
#include <QWheelEvent>

//just for testing
#include <QGraphicsSvgItem>
#include <QSvgRenderer>

using namespace Context;

ContextView *ContextView::s_instance = 0;
QString ContextView::s_wikiLocale = "en";

ContextView::ContextView()
    : QGraphicsView()
    , EngineObserver( EngineController::instance() )
    , m_lyricsBox( 0 )
    , m_dirtyLyricsPage( true )
    , m_HTMLSource( QString() )
    , m_lyricsVisible( false )
    , m_wikiBox( 0 )
    , m_wikiJob( 0 )
    , m_wikiCurrentEntry( QString() )
    , m_wikiCurrentUrl( QString() )
    , m_wikiBaseUrl( QString() )
    , m_dirtyWikiPage( true )
    , m_wikiHTMLSource( QString() )
    , m_wikiLanguages( QString() )
    , m_wiki( QString() )
    , m_wikiVisible( false )
//, m_wikiBackHistory( new QStringList() )
//, m_wikiForwardHistory( new QStringList() );
{
    s_instance = this; // we are a singleton class

    initiateScene();
    setAlignment( Qt::AlignTop );
    setRenderHints( QPainter::Antialiasing );
    setCacheMode( QGraphicsView::CacheBackground ); // this won't be changing regularly
    
    showHome();
}

void ContextView::initiateScene()
{
    m_contextScene = new QGraphicsScene( this );
    m_contextScene->setItemIndexMethod( QGraphicsScene::BspTreeIndex );
    m_contextScene->setBackgroundBrush( palette().highlight() );
    setScene( m_contextScene );
}

void ContextView::engineStateChanged( Engine::State state, Engine::State oldState )
{
    DEBUG_BLOCK
    Q_UNUSED( oldState );

    switch( state )
    {
        case Engine::Playing:
            showCurrentTrack();
            showLyrics( QString() ); // temporary, but we might as well show lyrics for now
        showWikipedia(); // lets show off the wikipedia box too
            break;

        case Engine::Empty:
            showHome();
            break;

        default:
            ;
    }
}

void ContextView::engineNewMetaData( const MetaBundle&, bool )
{
}

void ContextView::showHome()
{
    clear();

    /*
    FadingImageItem * fadeingImage = new FadingImageItem (QPixmap( KStandardDirs::locate("data", "amarok/images/splash_screen.jpg" ) ) );
    QColor color = palette().highlight();
    fadeingImage->setFadeColor( color );
    fadeingImage->setTargetAlpha( 200 );
    */

    /*
    QGraphicsPixmapItem *logoItem = new QGraphicsPixmapItem ( QPixmap( KStandardDirs::locate("data", "amarok/images/splash_screen.jpg" ) ) );

    GraphicsItemFader *logoFader = new GraphicsItemFader( logoItem, 0 );
//     logoFader->setTargetAlpha( 200 );
    logoFader->setFadeColor( palette().highlight() );
    logoFader->setDuration( 5000 );
    logoFader->setFPS( 30 );
    logoFader->setStartAlpha( 0 );
    logoFader->setTargetAlpha( 200 );

    addContextBox( logoFader );
    logoFader->startFading();
    */

    //test TextFader

   /* TextFader *textFader = new TextFader("Hello, World", 0);
    QFont font = textFader->font();
    font.setPointSize( 20 );
    textFader->setFont( font );

    textFader->setDuration( 5000 );
    textFader->setFPS( 30 );
    textFader->setStartAlpha( 0 );
    textFader->setTargetAlpha( 255 );

    addContextBox( textFader );
    textFader->startFading();*/

    /*
    IntroAnimation *introAnim = new IntroAnimation();

    connect( introAnim, SIGNAL( animationComplete() ), this, SLOT( introAnimationComplete() ) );

    debug() << "starting intro anim" << endl;

    introAnim->setFadeColor( palette().highlight() );
    addContextBox( introAnim );
    introAnim->startAnimation();
    */
    introAnimationComplete();
}



void ContextView::introAnimationComplete()
{
    clear();
    debug() << "introAnimationComplete!"  << endl;

    ContextBox *welcomeBox = new ContextBox();
    welcomeBox->setTitle( "Hooray, welcome to Amarok::ContextView!" );
    addContextBox( welcomeBox );


    AlbumBox *albumBox = new AlbumBox();
    albumBox->setTitle( "Your Newest Albums" );

    // because i don't know how to use the new QueryMaker class...
    QString query = "SELECT distinct(AL.name), AR.name, YR.name "
                    "FROM tags T "
                    "INNER JOIN album AL ON T.album = AL.id "
                    "INNER JOIN artist AR ON T.artist = AR.id "
                    "INNER JOIN year YR ON T.year = YR.id "
                    "WHERE T.sampler=0 "
                    "ORDER BY T.createdate DESC "
                     "LIMIT 5 OFFSET 0";
    QStringList values = CollectionDB::instance()->query( query, false );
    debug() << "Result count: " << values.count() << endl;

    for( QStringList::ConstIterator it = values.begin(), end = values.end(); it != end; ++it )
    {
        const QString album = *it;
        const QString artist = *++it;
        const QString year = *++it;
        const QString &cover = CollectionDB::instance()->albumImage( artist, album, false, 50 );
        debug() << "artist: " << artist << " album: " << album << " cover: " << cover << endl;
        albumBox->addAlbumInfo( cover, QString( "%1 - %2\n%3" ).arg( artist, album, year ) );
    }

    addContextBox( albumBox );

    //testing

    debug() << KStandardDirs::locate("data", "amarok/images/amarok_icon.svg" ) << endl;
    QGraphicsSvgItem * svg = new QGraphicsSvgItem( KStandardDirs::locate("data", "amarok/images/amarok_icon.svg" ) );
    svg->scale(0.5, 0.5 );
    addContextBox( svg );
}

void ContextView::showCurrentTrack()
{
    clear();

    MetaBundle bundle = EngineController::instance()->bundle();

    ContextBox *infoBox = new ContextBox();
    infoBox->setTitle( i18n("%1 - %2", bundle.title(), bundle.artist() ) );
    addContextBox( infoBox );

    CloudBox *relatedArtists = new CloudBox();
    relatedArtists->setTitle( i18n("Related Artists to %1", bundle.artist() ) );
    QStringList relations = CollectionDB::instance()->similarArtists( bundle.artist(), 10 );
    foreach( QString r, relations )
        relatedArtists->addText( r );

    addContextBox( relatedArtists );

    AlbumBox *albumBox = new AlbumBox();
    albumBox->setTitle( i18n("Albums By %1", bundle.artist() ) );

    int artistId = CollectionDB::instance()->artistID( bundle.artist() );
    // because i don't know how to use the new QueryMaker class...
    QString query = QString("SELECT distinct(AL.name), AR.name, YR.name "
            "FROM tags T "
            "INNER JOIN album AL ON T.album = AL.id "
            "INNER JOIN artist AR ON T.artist = AR.id "
            "INNER JOIN year YR ON T.year = YR.id "
            "WHERE AR.id = %1 "
            "ORDER BY YR.name DESC").arg( artistId );

    QStringList values = CollectionDB::instance()->query( query, false );
    debug() << "Result count: " << values.count() << endl;

    for( QStringList::ConstIterator it = values.begin(), end = values.end(); it != end; ++it )
    {
        QString album = *it;
        if( album.isEmpty() )
            album = i18n( "Unknown" );

        const QString artist = *++it;
        const QString year = *++it;
        const QString &cover = CollectionDB::instance()->albumImage( artist, album, false, 50 );
        debug() << "artist: " << artist << " album: " << album << " cover: " << cover << endl;
        albumBox->addAlbumInfo( cover, QString( "%1 - %2\n%3" ).arg( artist, album, year ) );
    }

    addContextBox( albumBox );
}

///////////////////////////////////////////////////////
////    Lyrics methods                           //////
///////////////////////////////////////////////////////
void ContextView::showLyrics( const QString& url )
{
    DEBUG_BLOCK
    
        debug() << url << endl;
    // NOTE: we check for if our lyrics box is visible at the end, 
    // once we populate it.
    
    m_lyricsBox = new GenericInfoBox();
    if( !m_dirtyLyricsPage )
    {
        if( !m_lyricsVisible )
        {
            addContextBox( m_lyricsBox );
            m_lyricsBox->ensureVisible();
            m_lyricsVisible = true;
        }
        //debug() << "lyrics already loaded" << endl; // lyrics are already loaded
        return;
    }
    
    QString lyrics = CollectionDB::instance()->getLyrics( EngineController::instance()->bundle().url().path() );
    // don't rely on caching for streams
    const bool cached = !lyrics.isEmpty() && !EngineController::engine()->isStream();
    QString title  = EngineController::instance()->bundle().title();
    QString artist = EngineController::instance()->bundle().artist();

    
    m_lyricsBox->setTitle( QString( "Lyrics of %1").arg( title ) );
    // magnatune cleaning
    if( title.contains("PREVIEW: buy it at www.magnatune.com", true) )
        title = title.remove(" (PREVIEW: buy it at www.magnatune.com)");
    if( artist.contains("PREVIEW: buy it at www.magnatune.com", true) )
        artist = artist.remove(" (PREVIEW: buy it at www.magnatune.com)");
    
    if ( title.isEmpty() ) {
        /* If title is empty, try to use pretty title.
           The fact that it often (but not always) has "artist name" together, can be bad,
           but at least the user will hopefully get nice suggestions. */
        QString prettyTitle = EngineController::instance()->bundle().prettyTitle();
        int h = prettyTitle.indexOf( '-' );
        if ( h != -1 )
        {
            title = prettyTitle.mid( h+1 ).trimmed();
            if( title.contains("PREVIEW: buy it at www.magnatune.com", true) )
                title = title.remove(" (PREVIEW: buy it at www.magnatune.com)");
            if ( artist.isEmpty() ) {
                artist = prettyTitle.mid( 0, h ).trimmed();
                if( artist.contains("PREVIEW: buy it at www.magnatune.com", true) )
                    artist = artist.remove(" (PREVIEW: buy it at www.magnatune.com)");
            }
            
        }
    }
    
    
    if( ( !cached || url == "reload" ) && ScriptManager::instance()->lyricsScriptRunning().isEmpty() ) 
    {
        const QStringList scripts = ScriptManager::instance()->lyricsScripts();
        lyrics =
            i18n( "Sorry, no lyrics script running.") + "<br />\n" +
            "<br /><div class='info'>\n"+
            i18n( "Available Lyrics Scripts:" ) + "<br />\n";
        oldForeach ( scripts ) {
            lyrics += QString( "<a href=\"runscript:%1\">%2</a><br />\n" ).arg( *it, *it );
        }
        lyrics += "<br />\n" + i18n( "Click on one of the scripts to run it, or use the Script Manager, to be able"
                                     " to see all the scripts, and download new ones from the Web." );
        lyrics += "<br /><div align='center'>\n"
            "<input type='button' onClick='window.location.href=\"show:scriptmanager\";' value='" +
            i18n( "Run Script Manager..." ) +
            "'></div><br /></div>\n";
        
        m_HTMLSource = QString (
                                 "<html><body>\n"
                                 "<div id='lyrics_box' class='box'>\n"
                                 "<div id='lyrics_box-header' class='box-header'>\n"
                                 "<span id='lyrics_box-header-title' class='box-header-title'>\n"
                                 + ( cached ? i18n( "Cached Lyrics" ) : i18n( "Lyrics" ) ) +
                                 "</span>\n"
                                 "</div>\n"
                                 "<div id='lyrics_box-body' class='box-body'>\n"
                                 + lyrics +
                                 "</div>\n"
                                 "</div>\n"
                                 "</body></html>\n"
                               );
        m_lyricsBox->setContents( m_HTMLSource );
        
        if( !m_lyricsVisible )
        {
            addContextBox( m_lyricsBox );
            m_lyricsBox->ensureVisible();
            m_lyricsVisible = true;
        }
        m_lyricsBox->ensureVisible();
        m_dirtyLyricsPage = false;
        // saveHtmlData(); // Send html code to file
        
        return;
    }

    
    if( cached && url.isEmpty() )
    {
        lyricsResult( lyrics.toUtf8(), true );
    }
    else
    {
        m_HTMLSource = QString (
                                 "<html><body>\n"
                                 "<div id='lyrics_box' class='box'>\n"
                                 "<div id='lyrics_box-header' class='box-header'>\n"
                                 "<span id='lyrics_box-header-title' class='box-header-title'>\n"
                                 + i18n( "Fetching Lyrics" ) +
                                 "</span>\n"
                                 "</div>\n"
                                 "<div id='lyrics_box-body' class='box-body'>\n"
                                 "<div class='info'><p>\n" + i18n( "Fetching Lyrics..." ) + "</p></div>\n"
                                 "</div>\n"
                                 "</div>\n"
                                 "</body></html>\n"
                               );
        m_lyricsBox->setContents( m_HTMLSource );
        //saveHtmlData(); // Send html code to file
        
        
        if( url.isNull() || url == "reload" )
        {
            debug() << "notifying without url" << endl;
            ScriptManager::instance()->notifyFetchLyrics( artist, title );
        } else
        {
            debug() << "notifying by url, url is: " << url  << endl;
            ScriptManager::instance()->notifyFetchLyricsByUrl( url );
        }
    }
    
    
    if( !m_lyricsVisible )
    {
        addContextBox( m_lyricsBox );
        m_lyricsBox->ensureVisible();
        m_lyricsVisible = true;
    }
}



void
ContextView::lyricsResult( QByteArray cXmlDoc, bool cached ) //SLOT
{
    DEBUG_BLOCK
    QDomDocument doc;
    QString xmldoc = QString::fromUtf8( cXmlDoc );
    if( !doc.setContent( xmldoc ) )
    {
        m_HTMLSource="";
        m_HTMLSource.append(
                             "<html><body>\n"
                             "<div id='lyrics_box' class='box'>\n"
                             "<div id='lyrics_box-header' class='box-header'>\n"
                             "<span id='lyrics_box-header-title' class='box-header-title'>\n"
                             + i18n( "Error" ) +
                             "</span>\n"
                             "</div>\n"
                             "<div id='lyrics_box-body' class='box-body'><p>\n"
                             + i18n( "Lyrics could not be retrieved because the server was not reachable." ) +
                             "</p></div>\n"
                             "</div>\n"
                             "</body></html>\n"
                           );
        m_lyricsBox->clearContents();
        m_lyricsBox->setContents( m_HTMLSource );
        //saveHtmlData(); // Send html code to file
        
        m_dirtyLyricsPage = false;
        
        return;
    }
    
    QString lyrics;
    
    QDomElement el = doc.documentElement();
    
    ScriptManager* const sm = ScriptManager::instance();
    KConfig spec( sm->specForScript( sm->lyricsScriptRunning() ),  KConfig::NoGlobals );
    spec.setGroup( "Lyrics" );
    
    
    if ( el.tagName() == "suggestions" )
    {
        
        
        const QDomNodeList l = doc.elementsByTagName( "suggestion" );
        
        if( l.length() ==0 )
        {
            lyrics = i18n( "Lyrics for track not found" );
        }
        else
        {
            lyrics = i18n( "Lyrics for track not found, here are some suggestions:" ) + "<br/><br/>\n";
            for( uint i = 0; i < l.length(); ++i ) {
                const QString url    = l.item( i ).toElement().attribute( "url" );
                const QString artist = l.item( i ).toElement().attribute( "artist" );
                const QString title  = l.item( i ).toElement().attribute( "title" );
                
                lyrics += "<a href='show:suggestLyric-" + url + "'>\n" + i18n("%1 - %2", artist, title );
                lyrics += "</a><br/>\n";
            }
        }
        /*lyrics += i18n( "<p>You can <a href=\"%1\">search for the lyrics</a> on the Web.</p>" )
            .arg( QString( m_lyricSearchUrl ).replace( QRegExp( "^http:" ), "externalurl:" ) ); */
    }
    else {
        lyrics = el.text();
        lyrics.replace( "\n", "<br/>\n" ); // Plaintext -> HTML
        
        const QString title      = el.attribute( "title" );
        const QString artist     = el.attribute( "artist" );
        const QString site       = spec.readEntry( "site" );
        const QString site_url   = spec.readEntry( "site_url" );
        
        lyrics.prepend( "<font size='2'><b>\n" + title + "</b><br/><u>\n" + artist+ "</font></u></font><br/>\n" );
        
        if( !cached ) {
            lyrics.append( "<br/><br/><i>\n" + i18n( "Powered by %1 (%2)", site, site_url ) + "</i>\n" );
            CollectionDB::instance()->setLyrics( EngineController::instance()->bundle().url().path(), xmldoc, EngineController::instance()->bundle().uniqueId() );
        }
    }
    
    m_HTMLSource="";
    m_HTMLSource.append(
                         "<html><body>\n"
                         "<div id='lyrics_box' class='box'>\n"
                         "<div id='lyrics_box-header' class='box-header'>\n"
                         "<span id='lyrics_box-header-title' class='box-header-title'>\n"
                         + ( cached ? i18n( "Cached Lyrics" ) : i18n( "Lyrics" ) ) +
                         "</span>\n"
                         "</div>\n"
                         "<div id='lyrics_box-body' class='box-body'>\n"
                         + lyrics +
                         "</div>\n"
                         "</div>\n"
                         "</body></html>\n"
                       );
    
    
    m_lyricsBox->setContents( m_HTMLSource );
    //Reset scroll
    
    // m_lyricsPage->view()->setContentsPos(0, 0);
    
    if( !m_lyricsVisible )
    {
        addContextBox( m_lyricsBox );
        m_lyricsBox->ensureVisible();
        m_lyricsVisible = true;
    }
    
    //saveHtmlData(); // Send html code to file
    
    //wikiExternalPageAction->setEnabled( !m_lyricCurrentUrl.isEmpty() );
    //m_lyricsToolBar->getButton( LYRICS_BROWSER )->setEnabled( !m_lyricCurrentUrl.isEmpty() );
    m_dirtyLyricsPage = false;
}

///////////////////////////////////////////////////////////////////////
// Wikipedia box
////////////////////////////////////////////////////////////////////////

QString
ContextView::wikiArtistPostfix()
{
    if( wikiLocale() == "en" )
        return " (band)";
    else if( wikiLocale() == "de" )
        return " (Band)";
    else
        return "";
}

QString
ContextView::wikiAlbumPostfix()
{
    if( wikiLocale() == "en" )
        return " (album)";
    else
        return "";
}

QString
ContextView::wikiTrackPostfix()
{
    if( wikiLocale() == "en" )
        return " (song)";
    else
        return "";
}
/*
void
ContextView::wikiConfigChanged( int /*activeItem ) // SLOT
{
    // keep in sync with localeList in wikiConfig
    QString text = m_wikiLocaleCombo->currentText();
    
    // NOTE what is this? need to check in .h
    m_wikiLocaleEdit->setEnabled( text == i18n("Other...") );
    
    if( text == i18n("English") )
        m_wikiLocaleEdit->setText( "en" );
    
    else if( text == i18n("German") )
        m_wikiLocaleEdit->setText( "de" );
    
    else if( text == i18n("French") )
        m_wikiLocaleEdit->setText( "fr" );
    
    else if( text == i18n("Polish") )
        m_wikiLocaleEdit->setText( "pl" );
    
    else if( text == i18n("Japanese") )
        m_wikiLocaleEdit->setText( "ja" );
    
    else if( text == i18n("Spanish") )
        m_wikiLocaleEdit->setText( "es" ); 
}

void
ContextView::wikiConfigApply() // SLOT
{
    const bool changed = m_wikiLocaleEdit->text() != wikiLocale();
    setWikiLocale( m_wikiLocaleEdit->text() );
    
    if ( changed && currentWidget() == m_wikiTab && !m_wikiCurrentEntry.isNull() )
    {
        m_dirtyWikiPage = true;
        showWikipediaEntry( m_wikiCurrentEntry );
    }
    
    showWikipedia();
}


void
ContextView::wikiConfig() // SLOT
{
    QStringList localeList;
    localeList
        << i18n( "English" )
        << i18n( "German" )
        << i18n( "French" )
        << i18n( "Polish" )
        << i18n( "Japanese" )
        << i18n( "Spanish" )
        << i18n( "Other..." );
    
    int index;
    
    if( wikiLocale() == "en" )
        index = 0;
    else if( wikiLocale() == "de" )
        index = 1;
    else if( wikiLocale() == "fr" )
        index = 2;
    else if( wikiLocale() == "pl" )
        index = 3;
    else if( wikiLocale() == "ja" )
        index = 4;
    else if( wikiLocale() == "es" )
        index = 5;
    else // other
        index = 6;
    
    m_wikiConfigDialog = new KDialog( this );
    
    m_wikiConfigDialog->setModal( true );
    m_wikiConfigDialog->setButtons( KDialog::Ok | KDialog::Cancel | KDialog::Apply );
    m_wikiConfigDialog->showButtonSeparator( true );
    
    
    kapp->setTopWidget( m_wikiConfigDialog );
    m_wikiConfigDialog->setCaption( KDialog::makeStandardCaption( i18n( "Wikipedia Locale" ) ) );
    KVBox *box = new KVBox( this );
    m_wikiConfigDialog->setMainWidget( box );
    
    m_wikiLocaleCombo = new QComboBox( box );
    m_wikiLocaleCombo->insertStringList( localeList );
    
    KHBox  *hbox       = new KHBox( box );
    QLabel *otherLabel = new QLabel( i18n( "Locale: " ), hbox );
    m_wikiLocaleEdit   = new QLineEdit( "en", hbox );
    
    otherLabel->setBuddy( m_wikiLocaleEdit );
    m_wikiLocaleEdit->setToolTip( i18n( "2-letter language code for your Wikipedia locale" ) );
    
    connect( m_wikiLocaleCombo,  SIGNAL( activated(int) ), SLOT( wikiConfigChanged(int) ) );
    connect( m_wikiConfigDialog, SIGNAL( applyClicked() ), SLOT( wikiConfigApply() ) );
    
    m_wikiLocaleEdit->setText( wikiLocale() );
    m_wikiLocaleCombo->setCurrentItem( index );
    wikiConfigChanged( index ); // a little redundant, but saves ugly code, and ensures the lineedit enabled status is correct
    
    m_wikiConfigDialog->setInitialSize( QSize( 240, 100 ) );
    const int result = m_wikiConfigDialog->exec();
    
    
    if( result == QDialog::Accepted )
        wikiConfigApply();
    
    delete m_wikiConfigDialog;
}
*/
QString
ContextView::wikiLocale()
{
    if( s_wikiLocale.isEmpty() )
        return QString( "en" );
    
    return s_wikiLocale;
}

void
ContextView::setWikiLocale( const QString &locale )
{
    AmarokConfig::setWikipediaLocale( locale );
    s_wikiLocale = locale;
}

QString
ContextView::wikiURL( const QString &item )
{
    return QString( "http://%1.wikipedia.org/wiki/" ).arg( wikiLocale() )
        + KUrl::toPercentEncoding( item, "/" );
}

void
ContextView::reloadWikipedia()
{
    m_wikiJob = NULL;
    showWikipediaEntry( m_wikiCurrentEntry, true );
}

void
ContextView::showWikipediaEntry( const QString &entry, bool replaceHistory )
{
    m_wikiCurrentEntry = entry;
    showWikipedia( wikiURL( entry ), false, replaceHistory );
}


void ContextView::showWikipedia( const QString &url, bool fromHistory, bool replaceHistory )
{
#if 0
    if( BrowserBar::instance()->currentBrowser() != this )
    {
        debug() << "current browser is not context, aborting showWikipedia()" << endl;
        m_dirtyWikiPage = true;
        return;
    }
#endif
    
    if ( !m_dirtyWikiPage || m_wikiJob ) return;
    
    m_wikiBox = new GenericInfoBox();
    // Disable the Open in a Browser button, because while loading it would open wikipedia main page.
    //m_wikiToolBar->setItemEnabled( WIKI_BROWSER, false );
    //wikiExternalPageAction->setEnabled( false );
    
    m_wikiBox->setTitle( QString( "Artist Info for %1" ).arg(  EngineController::instance()->bundle().artist() ) );
    m_wikiHTMLSource="";
    m_wikiHTMLSource.append(
                         "<html><body>\n"
                         "<div id='wiki_box' class='box'>\n"
                         "<div id='wiki_box-header' class='box-header'>\n"
                         "<span id='wiki_box-header-title' class='box-header-title'>\n"
                         + i18n( "Wikipedia" ) +
                         "</span>\n"
                         "</div>\n"
                         "<div id='wiki_box-body' class='box-body'>\n"
                         "<div class='info'><p>\n" + i18n( "Fetching Wikipedia Information" ) + " ...</p></div>\n"
                         "</div>\n"
                         "</div>\n"
                         "</body></html>\n"
                       );
    
    m_wikiBox->setContents( m_wikiHTMLSource );
    if( !m_wikiVisible )
    {
        addContextBox( m_wikiBox );
        m_wikiVisible = true;
    }
    
    if ( url.isEmpty() )
    {
        QString tmpWikiStr;
        
        if ( (EngineController::instance()->bundle().url().protocol() == "lastfm") ||
             (EngineController::instance()->bundle().url().protocol() == "daap") ||
             !EngineController::engine()->isStream() )
        {
            if ( !EngineController::instance()->bundle().artist().isEmpty() )
            {
                tmpWikiStr = EngineController::instance()->bundle().artist();
                tmpWikiStr += wikiArtistPostfix();
            }
            else if ( !EngineController::instance()->bundle().title().isEmpty() )
            {
                tmpWikiStr = EngineController::instance()->bundle().title();
            }
            else
            {
                tmpWikiStr = EngineController::instance()->bundle().prettyTitle();
            }
        }
        else
        {
            tmpWikiStr = EngineController::instance()->bundle().prettyTitle();
        }
        
        //Hack to make wiki searches work with magnatune preview tracks
        
        if ( tmpWikiStr.contains( "PREVIEW: buy it at www.magnatune.com" ) ) {
            tmpWikiStr = tmpWikiStr.remove(" (PREVIEW: buy it at www.magnatune.com)" );
            int index = tmpWikiStr.indexOf( '-' );
            if ( index != -1 ) {
                tmpWikiStr = tmpWikiStr.left (index - 1);
            }
            
        }
        m_wikiCurrentEntry = tmpWikiStr;
        
        m_wikiCurrentUrl = wikiURL( tmpWikiStr );
    }
    else
    {
        m_wikiCurrentUrl = url;
    }
    
    // Append new URL to history
    if ( replaceHistory )
    {
        m_wikiBackHistory.back() = m_wikiCurrentUrl;
    }
    else if ( !fromHistory ) {
        m_wikiBackHistory += m_wikiCurrentUrl;
        m_wikiForwardHistory.clear();
    }
    // Limit number of items in history
    if ( m_wikiBackHistory.count() > WIKI_MAX_HISTORY )
        m_wikiBackHistory.pop_front();
    
    m_wikiBaseUrl = m_wikiCurrentUrl.mid(0 , m_wikiCurrentUrl.indexOf("wiki/"));
    m_wikiJob = KIO::storedGet( m_wikiCurrentUrl, false, false );
    
    Amarok::StatusBar::instance()->newProgressOperation( m_wikiJob )
        .setDescription( i18n( "Fetching Wikipedia Information" ) );
    
    connect( m_wikiJob, SIGNAL( result( KJob* ) ), SLOT( wikiResult( KJob* ) ) );
}


void
ContextView::wikiArtistPage() //SLOT
{
    m_dirtyWikiPage = true;
    showWikipedia(); // Will fall back to title, if artist is empty(streams!).
}


void
ContextView::wikiAlbumPage() //SLOT
{
    m_dirtyWikiPage = true;
    showWikipediaEntry( EngineController::instance()->bundle().album() + wikiAlbumPostfix() );
}


void
ContextView::wikiTitlePage() //SLOT
{
    m_dirtyWikiPage = true;
    showWikipediaEntry( EngineController::instance()->bundle().title() + wikiTrackPostfix() );
}


void
ContextView::wikiExternalPage() //SLOT
{
    Amarok::invokeBrowser( m_wikiCurrentUrl );
}


void
ContextView::wikiResult( KJob* job ) //SLOT
{
    DEBUG_BLOCK
        
        if ( !job->error() == 0 && job == m_wikiJob )
        { // make sure its not the wrong job (e.g. wiki request for now changed song 
            m_wikiHTMLSource="";
            m_wikiHTMLSource.append(
                                 "<div id='wiki_box' class='box'>\n"
                                 "<div id='wiki_box-header' class='box-header'>\n"
                                 "<span id='wiki_box-header-title' class='box-header-title'>\n"
                                 + i18n( "Error" ) +
                                 "</span>\n"
                                 "</div>\n"
                                 "<div id='wiki_box-body' class='box-body'><p>\n"
                                 + i18n( "Artist information could not be retrieved because the server was not reachable." ) +
                                 "</p></div>\n"
                                 "</div>\n"
                               );
            m_wikiBox->clearContents();
            m_wikiBox->setContents( m_wikiHTMLSource );
            if( !m_wikiVisible )
            {
                addContextBox( m_wikiBox );
                m_wikiVisible = true;
            }
            m_dirtyWikiPage = false;
        //m_wikiPage = NULL; // FIXME: what for? leads to crashes
            
            warning() << "[WikiFetcher] KIO error! errno: " << job->error() << endl;
            return;
        }
    if ( job != m_wikiJob )
        return; //not the right job, so let's ignore it
    
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    m_wiki = QString( storedJob->data() );
    
    // Enable the Open in a Brower button, Disabled while loading, guz it would open wikipedia main page.
    //m_wikiToolBar->setItemEnabled( WIKI_BROWSER, true );
    //wikiExternalPageAction->setEnabled( true );
    
    // FIXME: Get a safer Regexp here, to match only inside of <head> </head> at least.
    if ( m_wiki.contains( "charset=utf-8"  ) ) {
        m_wiki = QString::fromUtf8( storedJob->data().data(), storedJob->data().size() );
    }
    
    if( m_wiki.indexOf( "var wgArticleId = 0" ) != -1 )
    {
        // article was not found
        if( m_wikiCurrentEntry.endsWith( wikiArtistPostfix() ) )
        {
            m_wikiCurrentEntry = m_wikiCurrentEntry.left( m_wikiCurrentEntry.length() - wikiArtistPostfix().length() );
            reloadWikipedia();
            return;
        }
        else if( m_wikiCurrentEntry.endsWith( wikiAlbumPostfix() ) )
        {
            m_wikiCurrentEntry = m_wikiCurrentEntry.left( m_wikiCurrentEntry.length() - wikiAlbumPostfix().length() );
            reloadWikipedia();
            return;
        }
        else if( m_wikiCurrentEntry.endsWith( wikiTrackPostfix() ) )
        {
            m_wikiCurrentEntry = m_wikiCurrentEntry.left( m_wikiCurrentEntry.length() - wikiTrackPostfix().length() );
            reloadWikipedia();
            return;
        }
    }
    
    //remove the new-lines and tabs(replace with spaces IS needed).
    m_wiki.replace( "\n", " " );
    m_wiki.replace( "\t", " " );
    
    m_wikiLanguages.clear();
    // Get the available language list
    if ( m_wiki.indexOf("<div id=\"p-lang\" class=\"portlet\">") != -1 )
    {
        m_wikiLanguages = m_wiki.mid( m_wiki.indexOf("<div id=\"p-lang\" class=\"portlet\">") );
        m_wikiLanguages = m_wikiLanguages.mid( m_wikiLanguages.indexOf("<ul>") );
        m_wikiLanguages = m_wikiLanguages.mid( 0, m_wikiLanguages.indexOf( "</div>" ) );
    }
    
    QString copyright;
    QString copyrightMark = "<li id=\"f-copyright\">";
    if ( m_wiki.indexOf( copyrightMark ) != -1 )
    {
        copyright = m_wiki.mid( m_wiki.indexOf(copyrightMark) + copyrightMark.length() );
        copyright = copyright.mid( 0, copyright.indexOf( "</li>" ) );
        copyright.replace( "<br />", QString() );
        //only one br at the beginning
        copyright.prepend( "<br />" );
    }
    
    // Ok lets remove the top and bottom parts of the page
    m_wiki = m_wiki.mid( m_wiki.indexOf( "<h1 class=\"firstHeading\">" ) );
    m_wiki = m_wiki.mid( 0, m_wiki.indexOf( "<div class=\"printfooter\">" ) );
    // Adding back license information
    m_wiki += copyright;
    m_wiki.append( "</div>" );
    m_wiki.replace( QRegExp("<h3 id=\"siteSub\">[^<]*</h3>"), QString() );
    
    m_wiki.replace( QRegExp( "<span class=\"editsection\"[^>]*>[^<]*<[^>]*>[^<]*<[^>]*>[^<]*</span>" ), QString() );
    
    m_wiki.replace( QRegExp( "<a href=\"[^\"]*\" class=\"new\"[^>]*>([^<]*)</a>" ), "\\1" );
    
    // Remove anything inside of a class called urlexpansion, as it's pointless for us
    m_wiki.replace( QRegExp( "<span class= *'urlexpansion'>[^(]*[(][^)]*[)]</span>" ), QString() );
    
    // Remove hidden table rows as well
    QRegExp hidden( "<tr *class= *[\"\']hiddenStructure[\"\']>.*</tr>", false );
    hidden.setMinimal( true ); //greedy behaviour wouldn't be any good!
    m_wiki.replace( hidden, QString() );
    
    // we want to keep our own style (we need to modify the stylesheet a bit to handle things nicely)
    m_wiki.replace( QRegExp( "style= *\"[^\"]*\"" ), QString() );
    m_wiki.replace( QRegExp( "class= *\"[^\"]*\"" ), QString() );
    // let's remove the form elements, we don't want them.
    m_wiki.replace( QRegExp( "<input[^>]*>" ), QString() );
    m_wiki.replace( QRegExp( "<select[^>]*>" ), QString() );
    m_wiki.replace( "</select>\n" , QString() );
    m_wiki.replace( QRegExp( "<option[^>]*>" ), QString() );
    m_wiki.replace( "</option>\n" , QString() );
    m_wiki.replace( QRegExp( "<textarea[^>]*>" ), QString() );
    m_wiki.replace( "</textarea>" , QString() );
    
    //first we convert all the links with protocol to external, as they should all be External Links.
    m_wiki.replace( QRegExp( "href= *\"http:" ), "href=\"externalurl:" );
    m_wiki.replace( QRegExp( "href= *\"/" ), "href=\"" +m_wikiBaseUrl );
    m_wiki.replace( QRegExp( "href= *\"#" ), "href=\"" +m_wikiCurrentUrl + '#' );
    
    m_wikiHTMLSource = "<html><body>\n";
    m_wikiHTMLSource.append(
                         "<div id='wiki_box' class='box'>\n"
                         "<div id='wiki_box-header' class='box-header'>\n"
                         "<span id='wiki_box-header-title' class='box-header-title'>\n"
                         + i18n( "Wikipedia Information" ) +
                         "</span>\n"
                         "</div>\n"
                         "<div id='wiki_box-body' class='box-body'>\n"
                         + m_wiki +
                         "</div>\n"
                         "</div>\n"
                       );
    if ( !m_wikiLanguages.isEmpty() )
    {
        m_wikiHTMLSource.append(
                             "<div id='wiki_box' class='box'>\n"
                             "<div id='wiki_box-header' class='box-header'>\n"
                             "<span id='wiki_box-header-title' class='box-header-title'>\n"
                             + i18n( "Wikipedia Other Languages" ) +
                             "</span>\n"
                             "</div>\n"
                             "<div id='wiki_box-body' class='box-body'>\n"
                             + m_wikiLanguages +
                             "</div>\n"
                             "</div>\n"
                           );
    }
    m_wikiHTMLSource.append( "</body></html>\n" );
    m_wikiBox->clearContents();
    m_wikiBox->setContents( m_wikiHTMLSource );
    if( !m_wikiVisible )
    {
        addContextBox( m_wikiBox );
        m_wikiVisible = true;
    }
    
    m_dirtyWikiPage = false;
    m_wikiJob = NULL;
}


void ContextView::scaleView( qreal factor )
{
    qreal scaleF = matrix().scale( factor, factor).mapRect(QRectF(0, 0, 1, 1)).width();
    if( scaleF < 0.07 || scaleF > 100 )
         return;

    scale( factor, factor );
}

void ContextView::wheelEvent( QWheelEvent *event )
{
    if( event->modifiers() & Qt::ControlModifier )
        scaleView( pow( (double)2, -event->delta() / 240.0) );
    else
        QGraphicsView::wheelEvent( event );
}

void ContextView::resizeEvent( QResizeEvent *event )
{
    QSize newSize = event->size();
    QList<QGraphicsItem*> items = m_contextScene->items();

    foreach( QGraphicsItem *item, items )
    {
        ContextBox *box = dynamic_cast<ContextBox*>( item );
        if( box )
            box->ensureWidthFits( newSize.width() );
    }
}

void ContextView::clear()
{
    
    if( m_lyricsVisible ) m_contextScene->removeItem( m_lyricsBox ); 
    if( m_lyricsVisible && m_lyricsBox != 0 ) delete m_lyricsBox;
    m_lyricsVisible = false;
    m_dirtyLyricsPage = true;
    if( m_wikiVisible ) m_contextScene->removeItem( m_wikiBox );
    if( m_wikiVisible && m_wikiBox != 0 ) delete m_wikiBox;
    m_wikiJob = 0;
    m_wikiVisible = false;
    m_dirtyWikiPage = true;
    delete m_contextScene;
    initiateScene();
    update();
}

void ContextView::addContextBox( QGraphicsItem *newBox, int after, bool fadeIn )
{
    if( !newBox || !m_contextScene )
        return;

    if( fadeIn )
    {
        GraphicsItemFader *fader = new GraphicsItemFader( newBox, 0 );
        fader->setDuration( 2500 );
        fader->setFPS( 30 );
        fader->setStartAlpha( 255 );
        fader->setTargetAlpha( 0 );
        fader->setFadeColor( palette().highlight().color() );
        fader->startFading();
        newBox = fader;
    }

    // For now, let's assume that all the items are listed in a vertical alignment
    // with a constant padding between the elements. Does this need to be more robust?
    QList<QGraphicsItem*> items = m_contextScene->items();
    qreal yposition = BOX_PADDING;

    foreach( QGraphicsItem* i, items )
        debug() << "bottom of a box: " << i->sceneBoundingRect().bottom() << endl;
    
        if( !items.isEmpty() )
    {
        // Since the items are returned in no particular order, we must sort the items first
        // based on the topmost edge of the box.
        // NOTE this is not what we want! we want to sort by bottom edge of box,
        // or else boxes that are higher than others but longer end up overlapping.
        // NOTE i haven't figured out how to do that yet :)
        qSort( items );

        /*foreach( QGraphicsItem* i, items )
            debug() << "bottom of a box: " << i->sceneBoundingRect().bottom() << endl;
        */
        
        if( after >= items.count() )
            after = -1;

        QGraphicsItem *afterItem = 0;

        // special case 'add-to-end' index, -1.
        if( after < 0 )
            afterItem = items.last();
        else
            afterItem = items.at( after );

        if( afterItem )
            yposition = afterItem->sceneBoundingRect().bottom() + BOX_PADDING;
    }

    debug() << "placing box at position: " << after << ", y position of box: " << yposition << endl;

    m_contextScene->addItem( newBox );
    newBox->setPos( BOX_PADDING, yposition );
}

bool ContextView::higherThan( const QGraphicsItem *i1, const QGraphicsItem *i2 )
{
    return ( i1->sceneBoundingRect().top() > i2->sceneBoundingRect().top() );
}

#include "contextview.moc"

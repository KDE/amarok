/***************************************************************************
 * copyright     : (C) 2007 Seb Ruiz <ruiz@kde.org>                        *
 *                 (C) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com> *
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

ContextView::ContextView()
    : QGraphicsView()
    , EngineObserver( EngineController::instance() )
    , m_lyricsBox( 0 )
    , m_dirtyLyricsPage( true )
    , m_HTMLSource( QString() )
    , m_lyricsVisible( false )
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
            showLyrics( QString() );
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

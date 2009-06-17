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


#include <KIcon>
#include <KStandardDirs>

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
    , m_reloadIcon( 0 )
    , m_css( 0 )
    , m_albumState( false )
    , m_artistState( false )
    , m_lyricsState( false )
    , m_contextMenu( 0 )
{
    setHasConfigurationInterface( false );
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

    paletteChanged( App::instance()->palette() );
    connect( The::paletteHandler(), SIGNAL( newPalette( const QPalette& ) ), SLOT(  paletteChanged( const QPalette &  ) ) );

    m_webView->page()->setLinkDelegationPolicy ( QWebPage::DelegateAllLinks );
    connect( m_webView->page(), SIGNAL( linkClicked( const QUrl & ) ) , this, SLOT( linkClicked ( const QUrl & ) ) );

    // make transparent so we can use qpainter translucency to draw the  background
    QPalette palette = m_webView->palette();
    palette.setBrush(QPalette::Base, Qt::transparent);
    m_webView->page()->setPalette(palette);
    m_webView->setAttribute(Qt::WA_OpaquePaintEvent, false);
    
    
    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_wikipediaLabel->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    m_wikipediaLabel->setFont( labelFont );
    m_wikipediaLabel->setText( i18n( "Wiki-Info" ) );

    QAction* reloadAction = new QAction( i18n( "Reload" ), this );
    reloadAction->setIcon( KIcon( "view-refresh" ) );
    reloadAction->setVisible( true );
    reloadAction->setEnabled( false );
    m_reloadIcon = addAction( reloadAction );

    connect( m_reloadIcon, SIGNAL( activated() ), this, SLOT( reloadWikipedia() ) );

    connectSource( "wikipedia-artist" );
    connectSource( "wikipedia-album" );
    connectSource( "wikipedia-lyrics" );
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

     QAction *reloadArtistAction = m_contextMenu -> addAction(i18n("Reload Artist Info"));
     reloadArtistAction -> setIcon( KIcon( "view-refresh" ) );
     reloadArtistAction -> setVisible( true );
     reloadArtistAction -> setEnabled( false );
     reloadArtistAction -> setText( "Reload Artist Info" );
     connect(reloadArtistAction,SIGNAL(triggered()),this,SLOT( reloadArtistInfo() ) );

     QAction *reloadLyricsAction = m_contextMenu -> addAction(i18n("Reload Lyrics Info"));
     reloadLyricsAction -> setIcon( KIcon( "view-refresh" ) );
     reloadLyricsAction -> setVisible( true );
     reloadLyricsAction -> setEnabled( false );
     reloadLyricsAction -> setText( "Reload Lyrics" );
     connect(reloadLyricsAction,SIGNAL(triggered()),this,SLOT( reloadLyricsInfo() ) );
     
     QAction *reloadAlbumAction = m_contextMenu -> addAction(i18n("Reload Album Info"));
     reloadAlbumAction -> setIcon( KIcon( "view-refresh" ) );
     reloadAlbumAction -> setVisible( true );
     reloadAlbumAction -> setEnabled( false );
     reloadAlbumAction -> setText( "Reload Album Info" );
     connect(reloadAlbumAction,SIGNAL(triggered()),this,SLOT( reloadAlbumInfo() ) );

     
     QAction *navigateArtistAction = m_contextMenu -> addAction(i18n("See Artist Info") );
     navigateArtistAction -> setVisible(false);
     navigateArtistAction -> setText( "See Artist Info" );
     connect(navigateArtistAction,SIGNAL(triggered()),this,SLOT( navigateToArtist() ) );

     QAction *navigateAlbumAction = m_contextMenu -> addAction(i18n("See Album Info") );
     navigateAlbumAction -> setVisible(false);
     navigateAlbumAction -> setText( "See Album Info" );
     connect(navigateAlbumAction,SIGNAL(triggered()),this,SLOT( navigateToAlbum() ) );

     QAction *navigateLyricsAction = m_contextMenu -> addAction(i18n("See Lyrics Info") );
     navigateLyricsAction -> setVisible(false);
     navigateLyricsAction -> setText( "See Lyrics Info" );
     connect(navigateLyricsAction,SIGNAL(triggered()),this,SLOT( navigateToLyrics() ) );
     
     QAction *compressArtistAction = m_contextMenu -> addAction(i18n("Compress Artist Info") );
     compressArtistAction -> setVisible(false);
     compressArtistAction -> setText( "Compress Artist Info" );
     connect(compressArtistAction,SIGNAL(triggered()),this,SLOT(compressArtistInfo() ) );
     
     QAction *compressAlbumAction = m_contextMenu -> addAction(i18n("Compress Album Info"));
     compressAlbumAction -> setVisible(false);
     compressAlbumAction -> setText( "Compress Album Info" );
     connect(compressAlbumAction,SIGNAL(triggered()),this,SLOT( compressAlbumInfo()));

     QAction *compressLyricsAction = m_contextMenu -> addAction(i18n("Compress Lyrics Info"));
     compressLyricsAction -> setVisible(false);
     compressLyricsAction -> setText( "Compress Lyrics Info" );
     connect(compressLyricsAction,SIGNAL(triggered()),this,SLOT( compressLyricsInfo()));
     
     QAction *expandArtistAction = m_contextMenu -> addAction(i18n("View More Artist Info"));
     expandArtistAction -> setVisible(false);
     expandArtistAction -> setText( "View More Artist Info" );
     connect(expandArtistAction,SIGNAL(triggered()),this,SLOT( expandArtistInfo() ) );
     
     QAction *expandAlbumAction = m_contextMenu -> addAction(i18n("View More Album Info" ) );
     expandAlbumAction -> setVisible(false);
     expandAlbumAction -> setText( "View More Album Info" );
     connect(expandAlbumAction,SIGNAL(triggered()),this,SLOT( expandAlbumInfo() ) );

     QAction *expandLyricsAction = m_contextMenu -> addAction(i18n("View More Lyrics Info" ) );
     expandLyricsAction -> setVisible(false);
     expandLyricsAction -> setText( "View More Lyrics Info" );
     connect(expandLyricsAction,SIGNAL(triggered()),this,SLOT( expandLyricsInfo() ) );

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
      
}

void
WikiApplet::linkClicked( const QUrl &pageurl )
{
    DEBUG_BLOCK
    debug() << "URL: " << pageurl;
    QString m = pageurl.toString();
    if(m.contains("artist") )
    {
     
       m_artistState = m_artistState ?false : true;
       updateHappening();
       navigateToArtist();
       m_webView -> toggleAction("Artist",m_artistState);
       return;
    }
    else if( m.contains("album") )
    {
        m_albumState = m_albumState ? false : true;
        updateHappening();
        navigateToAlbum();
        m_webView -> toggleAction("Album",m_albumState);
        return;
    }
   
    else if( m.contains("lyrics") )
    {
        m_lyricsState = m_lyricsState ? false : true;
        updateHappening();
        navigateToLyrics();
        m_webView -> toggleAction("Lyrics",m_lyricsState);
        return;
    }   
    QDesktopServices::openUrl( pageurl.toString() );
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
      

void WikiApplet::constraintsEvent( Plasma::Constraints constraints )
{

    prepareGeometryChange();
    
    float textWidth = m_wikipediaLabel->boundingRect().width();
    float offsetX =  ( boundingRect().width() - textWidth ) / 2;

    m_wikipediaLabel->setPos( offsetX, standardPadding() + 2 );

    m_webView->setPos( standardPadding(), m_wikipediaLabel->pos().y() + m_wikipediaLabel->boundingRect().height() + standardPadding() );
    m_webView->resize( boundingRect().width() - 2 * standardPadding(), boundingRect().height() - m_webView->pos().y() - standardPadding() );

    m_reloadIcon->setPos( size().width() - m_reloadIcon->size().width() - standardPadding(), standardPadding() );
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
}

void WikiApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data ) // SLOT
{
        DEBUG_BLOCK
        debug() << "The Name of the Source is : "<< name;
        if( data.size() == 0 ) return;
        
        if( name.contains( "artist" ) )
            updateArtistInfo(data);
        if(name .contains( "album" ) )
            updateAlbumInfo(data);
        if(name.contains( "lyrics" ) )
            updateLyricsInfo(data);
          
      updateHappening();
      if( m_reloadIcon->action() && !m_reloadIcon->action()->isEnabled() )
        {
            m_reloadIcon->action()->setEnabled( true );
            //for some reason when we enable the action suddenly the icon has the text "..."
            m_reloadIcon->action()->setText( "" );
        }

}

void WikiApplet::updateArtistInfo( const Plasma :: DataEngine :: Data& data )
{
    DEBUG_BLOCK
    
    if( data.contains( "title" ) )
    {
        m_artistHtml = "<h3 id=\"artist\">";
        m_artistHtml += data["title"].toString();
        m_artistHtml += "</h3>";
        m_compressedArtistHtml = m_artistHtml;
        m_artistTitle = data[ "title" ].toString();
    }
    else
        m_artistTitle.clear();
       
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
    }
    else if(data.contains( "message" ) )
    {     m_artistHtml = m_compressedArtistHtml += data[ "message" ] .toString();
          m_webView -> setDefaultActions("Artist");
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
    {
        m_albumHtml = "<h3 id = \"album\">";
        m_albumHtml += data["title"].toString();
        m_albumHtml += "</h3>";
        m_compressedAlbumHtml = m_albumHtml;
        m_albumTitle = data[ "title" ].toString();
    }
     else
        m_albumTitle.clear();
     
    if( data.contains( "page" ) )
    {    
            m_albumHtml += data[ "page" ].toString();
            m_albumHtml += "<p><a href = \"http:\\\\www.wikialbum.amarok\">View Less...</a></p>";
            int startIndex = m_albumHtml.indexOf("<p>");
            int endIndex = m_albumHtml.indexOf("</p>")+3;
            m_compressedAlbumHtml += m_albumHtml.mid(startIndex,endIndex-startIndex+1);
            m_compressedAlbumHtml += "<p><a href= \"http:\\\\www.wikialbum.amarok\">View More...</a></p>";
            m_webView -> enableViewActions(QString("Album"));
    }
    else if( data.contains( "message" ))
    {    m_albumHtml = m_compressedAlbumHtml += data["message"].toString();
          m_webView -> setDefaultActions("Album");
          m_albumState = false;
    }
    if( data.contains( "label" ) )
    {    m_label = data[ "label" ].toString() + ':';
    }
    else
        m_label.clear();
}


void WikiApplet :: updateLyricsInfo( const Plasma :: DataEngine :: Data& data )
{
         
        QTextDocument w;
        int flag = 0;
        m_lyricsHtml.clear();
        m_compressedLyricsHtml.clear();
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
                    //debug() << "parsing suggestion:" << sug;
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
        else if( data.contains( "notfound" ) )
        {
         
            w.setPlainText( i18n( "There were no lyrics found for this track" ) );
            flag = 1;
        }
        m_lyricsHtml += w.toHtml();
         if(flag)
         {
             m_compressedLyricsHtml = m_lyricsHtml;
             m_webView -> setDefaultActions("Lyrics");
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
            }
            else
            {    m_webView -> setDefaultActions("Lyrics");
                 m_compressedLyricsHtml = m_lyricsHtml;
               
            }
         }
	        
         
                 
        setPreferredSize( (int)size().width(), (int)size().height() );
        updateConstraints();
        update();
            
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

QSizeF WikiApplet::sizeHint( Qt::SizeHint which, const QSizeF & constraint ) const
{
        // ask for rest of CV height
    return QSizeF( QGraphicsWidget::sizeHint( which, constraint ).width(), -1 );
}

void
WikiApplet::reloadWikipedia()
{
    DEBUG_BLOCK
   reloadArtistInfo();
   reloadAlbumInfo();
   reloadLyricsInfo();
   
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
WikiApplet :: updateHappening()
{
      DEBUG_BLOCK
      QString display;

      if(m_lyricsState == false)
        display+= m_compressedLyricsHtml;
      else
          display+= m_lyricsHtml;
      if(m_artistState == false)
           display += m_compressedArtistHtml;
      else
          display += m_artistHtml;
      if(m_albumState == false)
          display += m_compressedAlbumHtml;
      else
          display += m_albumHtml;

      m_webView -> setHtml(display);
}

// My Web View class Definition begins here

MyWebView :: MyWebView ( QGraphicsItem *parent ) : Plasma :: WebView(parent)
{
    m_contextMenu = 0;
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

      m_contextMenu -> exec(event -> screenPos());
}

void MyWebView :: toggleAction(const QString s,bool status )
{
    QList<QAction*> actionList = m_contextMenu -> actions();
    if( !(s == "Artist" || s == "Album" || s == "Lyrics") )
      return;
    
    if(status)
    {
        foreach(QAction *action,actionList)
        {
            if( action -> text().contains(s)&& action->text().contains("Compress") )
                action -> setVisible(true);
            if( action -> text().contains(s) && action -> text().contains("More" ) )
                action -> setVisible(false);
        }
    }
    else
    {
        foreach(QAction *action,actionList)
        {
            if( action -> text().contains(s)&& action->text().contains("Compress") )
                action -> setVisible(false);
            if( action -> text().contains(s) && action -> text().contains("More" ) )
                action -> setVisible(true);
        }
    }
        
   
    
}

void MyWebView :: enableViewActions(const QString s)
{
    QList<QAction*> actionList = m_contextMenu -> actions();
     if( !(s == "Artist" || s == "Album" || s == "Lyrics") )
      return;
   
        foreach(QAction *action,actionList)
        {
            if( action -> text().contains(s)&& action->text().contains("Compress") )
                action -> setVisible(false);
            else if(action -> text().contains(s))
                action -> setVisible(true);
        }
   
}

void MyWebView :: setDefaultActions(const QString s)
{
      QList<QAction*> actionList = m_contextMenu -> actions();
      if( !(s == "Artist" || s == "Album" || s == "Lyrics") )
      return;

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

void MyWebView :: enableNavigateAction(const QString s)
{
     QList<QAction*> actionList = m_contextMenu -> actions();
     if( !(s == "Artist" || s == "Album" || s == "Lyrics") )
      return;

     foreach(QAction *action,actionList)
     {
          if( action -> text().contains(s)&& action->text().contains("See") )
                action -> setVisible(true);         
     }
}
#include "WikiApplet.moc"


/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
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

#include "WikipediaApplet.h"

#include "Amarok.h"
#include "App.h"
#include "Debug.h"
#include "context/Svg.h"
#include "context/ContextView.h"
#include "EngineController.h"
#include "PaletteHandler.h"

#include <Plasma/Theme>
#include <plasma/widgets/webview.h>
#include <plasma/widgets/iconwidget.h>
#include <plasma/widgets/pushbutton.h>

#include <KIcon>
#include <KGlobalSettings>
#include <KConfigDialog>
#include <KPushButton>
#include <KStandardDirs>

#include <QAction>
#include <QDesktopServices>
#include <QGraphicsSimpleTextItem>
#include <QPainter>
#include <QMenu>
#include <QWebHistory>
#include <QWebPage>


WikipediaApplet::WikipediaApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_aspectRatio( 0 )
    , m_headerAspectRatio( 0.0 )
    , m_wikipediaLabel( 0 )
    , m_webView( 0 )
    , m_backwardIcon( 0 )
    , m_forwardIcon( 0 )
    , m_artistIcon( 0 )
    , m_albumIcon( 0 )
    , m_trackIcon( 0 )
    , m_settingsIcon( 0 )
    , m_reloadIcon( 0 )
    , m_css( 0 )
    , m_gotMessage( 0 )
{
    setHasConfigurationInterface( true );
    setBackgroundHints( Plasma::Applet::NoBackground );
}

WikipediaApplet::~ WikipediaApplet()
{
    delete m_webView;
    delete m_css;
}

void
WikipediaApplet::init()
{   
    m_wikipediaLabel = new QGraphicsSimpleTextItem( this );

    m_webView = new Plasma::WebView( this );
    m_webView->setAttribute( Qt::WA_NoSystemBackground );

    // ask for all the CV height
    resize( 500, -1 );

    paletteChanged( App::instance()->palette() );
    connect( The::paletteHandler(), SIGNAL( newPalette( const QPalette& ) ), SLOT(  paletteChanged( const QPalette &  ) ) );

    m_webView->page()->setLinkDelegationPolicy ( QWebPage::DelegateAllLinks );
    connect( m_webView->page(), SIGNAL( linkClicked( const QUrl & ) ), SLOT( linkClicked ( const QUrl & ) ) );

    // make transparent so we can use qpainter translucency to draw the  background
    QPalette palette = m_webView->palette();
    palette.setBrush(QPalette::Base, Qt::transparent);
    m_webView->page()->setPalette(palette);   
    m_webView->setAttribute(Qt::WA_OpaquePaintEvent, false);
    
    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_wikipediaLabel->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    m_wikipediaLabel->setFont( labelFont );
    m_wikipediaLabel->setText( i18n( "Wikipedia" ) );

    QAction* backwardAction = new QAction( this );
    backwardAction->setIcon( KIcon( "go-previous" ) );
    backwardAction->setEnabled( false );
    backwardAction->setText( i18n( "Back" ) );
    m_backwardIcon = addAction( backwardAction );
    connect( backwardAction, SIGNAL( activated() ), this, SLOT( goBackward() ) );
    
    QAction* forwardAction = new QAction( this );
    forwardAction->setIcon( KIcon( "go-next" ) );
    forwardAction->setEnabled( false );
    forwardAction->setText( i18n( "Forward" ) );
    m_forwardIcon = addAction( forwardAction );
    connect( m_forwardIcon, SIGNAL( activated() ), this, SLOT( goForward() ) );

    QAction* artistAction = new QAction( this );
    artistAction->setIcon( KIcon( "filename-artist-amarok" ) );
    artistAction->setEnabled( false );
    artistAction->setText( i18n( "Artist" ) );
    m_artistIcon = addAction( artistAction );
    connect( m_artistIcon, SIGNAL( activated() ), this, SLOT( gotoArtist() ) );
    
    QAction* albumAction = new QAction( this );
    albumAction->setIcon( KIcon( "filename-album-amarok" ) );
    albumAction->setEnabled( false );
    albumAction->setText( i18n( "Album" ) );
    m_albumIcon = addAction( albumAction );
    connect( m_albumIcon, SIGNAL( activated() ), this, SLOT( gotoAlbum() ) );

    QAction* trackAction = new QAction( this );
    trackAction->setIcon( KIcon( "filename-title-amarok" ) );
    trackAction->setEnabled( false );
    trackAction->setText( i18n( "Track" ) );
    m_trackIcon = addAction( trackAction );
    connect( m_trackIcon, SIGNAL( activated() ), this, SLOT( gotoTrack() ) );

    QAction* langAction = new QAction( this );
    langAction->setIcon( KIcon( "preferences-system" ) );
    langAction->setEnabled( true );
    langAction->setText( i18n( "Settings" ) );
    m_settingsIcon = addAction( langAction );
    connect( m_settingsIcon, SIGNAL( activated() ), this, SLOT( switchLang() ) );
    
    QAction* reloadAction = new QAction( this );
    reloadAction->setIcon( KIcon( "view-refresh" ) );
    reloadAction->setEnabled( false );
    reloadAction->setText( i18n( "Reload" ) );
    m_reloadIcon = addAction( reloadAction );
    connect( m_reloadIcon, SIGNAL( activated() ), this, SLOT( reloadWikipedia() ) );

    connectSource( "wikipedia" );
    connect( dataEngine( "amarok-wikipedia" ), SIGNAL( sourceAdded( const QString & ) ), SLOT( connectSource( const QString & ) ) );
    
    constraintsEvent();

    // Read config and inform the engine.
    KConfigGroup config = Amarok::config("Wikipedia Applet");
    m_wikiPreferredLang = config.readEntry( "PreferredLang", "aut" );
    dataEngine( "amarok-wikipedia" )->query( QString( "wikipedia:AMAROK_TOKEN:lang:AMAROK_TOKEN:" ) + m_wikiPreferredLang );
}

void
WikipediaApplet::connectSource( const QString &source )
{
    if( source == "wikipedia" )
        dataEngine( "amarok-wikipedia" )->connectSource( "wikipedia", this );
}

void
WikipediaApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints );
    
    prepareGeometryChange();
    const float textWidth = m_wikipediaLabel->boundingRect().width();
    const float offsetX =  ( boundingRect().width() - textWidth ) / 2;

    m_wikipediaLabel->setPos( offsetX, standardPadding() + 2 );

    m_webView->setPos( standardPadding(), m_wikipediaLabel->pos().y() + m_wikipediaLabel->boundingRect().height() + standardPadding() );
    m_webView->resize( boundingRect().width() - 2 * standardPadding(), boundingRect().height() - m_webView->pos().y() - standardPadding() );
    
    // Icon positionning
    float iconWidth = m_backwardIcon->size().width();
    m_backwardIcon->setPos( size().width() - 7 * iconWidth - 6 * standardPadding(), standardPadding() );
    m_forwardIcon->setPos( size().width() - 6 * iconWidth - 6 * standardPadding(), standardPadding() );
    
    m_artistIcon->setPos( size().width() - 5 * iconWidth - 4 * standardPadding(), standardPadding() );
    m_albumIcon->setPos( size().width() - 4 * iconWidth - 4 * standardPadding(), standardPadding() );
    m_trackIcon->setPos( size().width() - 3 * iconWidth - 4 * standardPadding(), standardPadding() );

    m_reloadIcon->setPos( size().width() - 2 * iconWidth - 2 * standardPadding(), standardPadding() );
    m_settingsIcon->setPos( size().width() - iconWidth - standardPadding(), standardPadding() );
}

bool
WikipediaApplet::hasHeightForWidth() const
{
    return true;
}

qreal
WikipediaApplet::heightForWidth( qreal width ) const
{
    return width * m_aspectRatio;
}

void
WikipediaApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data ) // SLOT
{
    Q_UNUSED( name )

    if( data.size() == 0 )
        return;

    if( data.contains("busy") )
    {
        m_webView->hide();
        setBusy( true );
        return;
    }
    else
    {
        m_webView->show();
        setBusy( false );
    }
    
    if( data.contains( "page" ) )
    {
        if ( m_current.page == data[ "page" ].toString() && !m_gotMessage)
            return;
        
        // save last page, useful when you are reading but the song changes
        if ( !m_current.page.isEmpty() )
        {
            m_historyBack.push_front( m_current );
            while ( m_historyBack.size() > 20 )
                m_historyBack.pop_back();

            if ( m_backwardIcon->action() && !m_backwardIcon->action()->isEnabled() )
                m_backwardIcon->action()->setEnabled( true );

        }
        m_current.page = data[ "page" ].toString();
        m_current.url = KUrl( data[ "url" ].toString() );
        m_webView->setHtml( m_current.page, m_current.url );
        m_gotMessage = false;
        m_historyForward.clear();

        if ( m_forwardIcon->action() && m_forwardIcon->action()->isEnabled() )
            m_forwardIcon->action()->setEnabled( false );
    }
    
    if( data.contains( "message" ) )
    {
        m_webView->setHtml( data[ "message" ].toString(), KUrl( QString() ) ); // set data
        m_gotMessage = true; // we have a message and don't want to save it in history
    }

    if( m_reloadIcon->action() && !m_reloadIcon->action()->isEnabled() )
        m_reloadIcon->action()->setEnabled( true );

    if( m_artistIcon->action() && !m_artistIcon->action()->isEnabled() )
        m_artistIcon->action()->setEnabled( true );

    if( m_albumIcon->action() && !m_albumIcon->action()->isEnabled() )
        m_albumIcon->action()->setEnabled( true );

    if( m_trackIcon->action() && !m_trackIcon->action()->isEnabled() )
        m_trackIcon->action()->setEnabled( true );
}

void
WikipediaApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option )
    Q_UNUSED( contentsRect )

    p->setRenderHint( QPainter::Antialiasing );

    addGradientToAppletBackground( p );

    // draw rounded rect around title
    drawRoundedRectAroundText( p, m_wikipediaLabel );

    //draw background of wiki text
    p->save();

    // HACK
    // sometimes paint is done before the updateconstraints call
    // so m_webview bounding rect is not yet correct
    QRectF wikiRect(
    QPointF( standardPadding(), m_wikipediaLabel->pos().y() + m_wikipediaLabel->boundingRect().height() + standardPadding() ),
                    QSizeF( boundingRect().width() - 2 * standardPadding(), boundingRect().height() - m_webView->pos().y() - standardPadding() ) );
                    
    wikiRect.moveTopLeft( m_webView->pos() );
    QPainterPath round;
    round.addRoundedRect( wikiRect, 5, 5 );
    p->fillPath( round , The::paletteHandler()->backgroundColor() );
    p->restore(); 
}

void
WikipediaApplet::goBackward()
{
    DEBUG_BLOCK

    if( !m_historyBack.empty() )
    {
        m_historyForward.push_front( m_current );
        m_current =  m_historyBack.front();
        m_historyBack.pop_front();
        m_webView->setHtml( m_current.page, m_current.url );

        if( m_forwardIcon->action() && !m_forwardIcon->action()->isEnabled() )
            m_forwardIcon->action()->setEnabled( true );

        if ( m_historyBack.empty() && m_backwardIcon->action()->isEnabled() )
            m_backwardIcon->action()->setEnabled( false );
    }
}

void
WikipediaApplet::goForward()
{
    DEBUG_BLOCK

    if( !m_historyForward.empty() )
    {
        m_historyBack.push_front( m_current );
        m_current = m_historyForward.front();
        m_historyForward.pop_front();
        m_webView->setHtml( m_current.page , m_current.url );
        
        if( m_backwardIcon->action() && !m_backwardIcon->action()->isEnabled() )
            m_backwardIcon->action()->setEnabled( true );
        
        if ( m_historyForward.empty() && m_forwardIcon->action()->isEnabled() )
            m_forwardIcon->action()->setEnabled( false );
    }
}

void
WikipediaApplet::gotoArtist()
{
    DEBUG_BLOCK
    dataEngine( "amarok-wikipedia" )->query( "wikipedia:AMAROK_TOKEN:goto:AMAROK_TOKEN:artist" );
}

void
WikipediaApplet::gotoAlbum()
{
    DEBUG_BLOCK
    dataEngine( "amarok-wikipedia" )->query( "wikipedia:AMAROK_TOKEN:goto:AMAROK_TOKEN:album" );
}

void
WikipediaApplet::gotoTrack()
{
    DEBUG_BLOCK
    dataEngine( "amarok-wikipedia" )->query( "wikipedia:AMAROK_TOKEN:goto:AMAROK_TOKEN:track" );
}

void
WikipediaApplet::linkClicked( const QUrl &url )
{
    DEBUG_BLOCK
    if ( url.toString().contains( "wikipedia.org/" ) )
    {
        dataEngine( "amarok-wikipedia" )->query( QString( "wikipedia:AMAROK_TOKEN:get:AMAROK_TOKEN:" ) + url.toString() );
        if( m_backwardIcon->action() && !m_backwardIcon->action()->isEnabled() )
            m_backwardIcon->action()->setEnabled( true );

        m_historyForward.clear();
        if( m_forwardIcon->action() && m_forwardIcon->action()->isEnabled() )
            m_forwardIcon->action()->setEnabled( false );
    }
    else
        QDesktopServices::openUrl( url.toString() );
}

void
WikipediaApplet::reloadWikipedia()
{
    DEBUG_BLOCK
    dataEngine( "amarok-wikipedia" )->query( "wikipedia:AMAROK_TOKEN:reload" );
}

void
WikipediaApplet::switchLang()
{
    DEBUG_BLOCK
    showConfigurationInterface();
}

void
WikipediaApplet::switchToLang(QString lang)
{
    DEBUG_BLOCK
    // TODO change this b/c it's BAAADDD !!!
    if (lang == i18n("Automatic") )
        m_wikiPreferredLang = "aut";
    
    else if (lang == i18n("English") )
        m_wikiPreferredLang = "en";
    
    else if (lang == i18n("French") )
        m_wikiPreferredLang = "fr";
    
    else if (lang == i18n("German") )
        m_wikiPreferredLang = "de";

    dataEngine( "amarok-wikipedia" )->query( QString( "wikipedia:AMAROK_TOKEN:lang:AMAROK_TOKEN:" ) + m_wikiPreferredLang );

    KConfigGroup config = Amarok::config("Wikipedia Applet");
    config.writeEntry( "PreferredLang", m_wikiPreferredLang );
    dataEngine( "amarok-wikipedia" )->query( QString( "wikipedia:AMAROK_TOKEN:lang:AMAROK_TOKEN:" ) + m_wikiPreferredLang );
}

void
WikipediaApplet::createConfigurationInterface( KConfigDialog *parent )
{
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
    
    parent->addPage( settings, i18n( "Wikipedia Settings" ), "preferences-system");
    connect( ui_Settings.comboBox, SIGNAL( currentIndexChanged( QString ) ), this, SLOT( switchToLang( QString ) ) );
}

void
WikipediaApplet::paletteChanged( const QPalette & palette )
{
    // read css, replace color placeholders, write to file, load into page
    QFile file( KStandardDirs::locate("data", "amarok/data/WikipediaCustomStyle.css" ) );
    if( file.open(QIODevice::ReadOnly | QIODevice::Text) )
    {
        QString contents = QString( file.readAll() );

        const QColor bg = The::paletteHandler()->backgroundColor();
        contents.replace( "{text_background_color}", bg.name() );
        contents.replace( "{text_color}", palette.text().color().name() );
        contents.replace( "{link_color}", palette.link().color().name() );
        contents.replace( "{link_hover_color}", palette.linkVisited().color().name() );

        const QString abgName = The::paletteHandler()->alternateBackgroundColor().name();
        contents.replace( "{shaded_text_background_color}", abgName );
        contents.replace( "{table_background_color}", abgName );
        contents.replace( "{border_color}", abgName );
        contents.replace( "{headings_background_color}", abgName );

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

#include "WikipediaApplet.moc"


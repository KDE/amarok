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

#define DEBUG_PREFIX "WikipediaApplet"

#include "WikipediaApplet.h"

#include "core/support/Amarok.h"
#include "App.h"
#include "core/support/Debug.h"
#include "context/Svg.h"
#include "context/ContextView.h"
#include "EngineController.h"
#include "PaletteHandler.h"
#include "widgets/TextScrollingWidget.h"

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
#include <QPainter>
#include <QMenu>
#include <QWebHistory>
#include <QWebPage>
#include <QWebFrame>

class WikipediaAppletPrivate
{
private:
    WikipediaApplet *const q_ptr;
    Q_DECLARE_PUBLIC( WikipediaApplet )

public:
    WikipediaAppletPrivate( WikipediaApplet *parent )
        : q_ptr( parent )
        , css( 0 )
        , albumIcon( 0 )
        , artistIcon( 0 )
        , backwardIcon( 0 )
        , forwardIcon( 0 )
        , reloadIcon( 0 )
        , settingsIcon( 0 )
        , trackIcon( 0 )
        , webView( 0 )
        , wikipediaLabel( 0 )
        , gotMessage( 0 )
        , aspectRatio( 0 )
    {}
    ~WikipediaAppletPrivate() {}

    // functions
    qint64 writeStyleSheet( const QByteArray &css );

    // private slots
    void _connectSource( const QString &source );
    void _linkClicked( const QUrl &url );

    void _goBackward();
    void _goForward();
    void _gotoArtist();
    void _gotoAlbum();
    void _gotoTrack();

    void _switchLang();
    void _switchToLang( const QString &lang );
    void _reloadWikipedia();

    void _paletteChanged( const QPalette &palette );

    // data members
    struct HistoryItem
    {
        KUrl url;
        QString page;
    };

    KTemporaryFile *css;
    Plasma::IconWidget *albumIcon;
    Plasma::IconWidget *artistIcon;
    Plasma::IconWidget *backwardIcon;
    Plasma::IconWidget *forwardIcon;
    Plasma::IconWidget *reloadIcon;
    Plasma::IconWidget *settingsIcon;
    Plasma::IconWidget *trackIcon;
    Plasma::WebView *webView;
    QList<HistoryItem> historyBack;
    QList<HistoryItem> historyForward;
    HistoryItem current;
    QString preferredLang;
    TextScrollingWidget *wikipediaLabel;
    Ui::wikipediaSettings ui_Settings;
    bool gotMessage;
    qreal aspectRatio;
};

qint64
WikipediaAppletPrivate::writeStyleSheet( const QByteArray &data )
{
    delete css;
    css = new KTemporaryFile();
    css->setSuffix( ".css" );
    qint64 written( -1 );
    if( css->open() )
    {
        written = css->write( data );
        // NOTE shall we keep this commented out, and bring it back later or the base64 is just what we need ?
        //   QString filename = css->fileName();
        css->close(); // flush buffer to disk
        //   debug() << "set user stylesheet to:" << "file://" + filename;
        //   webView->page()->settings()->setUserStyleSheetUrl( "file://" + filename );
    }
    return written;
}

void
WikipediaAppletPrivate::_connectSource( const QString &source )
{
    Q_Q( WikipediaApplet );
    if( source == "wikipedia" )
        q->dataEngine( "amarok-wikipedia" )->connectSource( "wikipedia", q );
}

void
WikipediaAppletPrivate::_goBackward()
{
    DEBUG_BLOCK
    if( !historyBack.empty() )
    {
        historyForward.push_front( current );
        current =  historyBack.front();
        historyBack.pop_front();
        webView->setHtml( current.page, current.url );

        if( forwardIcon->action() && !forwardIcon->action()->isEnabled() )
            forwardIcon->action()->setEnabled( true );

        if( historyBack.empty() && backwardIcon->action()->isEnabled() )
            backwardIcon->action()->setEnabled( false );
    }
}

void
WikipediaAppletPrivate::_goForward()
{
    DEBUG_BLOCK
    if( !historyForward.empty() )
    {
        historyBack.push_front( current );
        current = historyForward.front();
        historyForward.pop_front();
        webView->setHtml( current.page , current.url );

        if( backwardIcon->action() && !backwardIcon->action()->isEnabled() )
            backwardIcon->action()->setEnabled( true );

        if( historyForward.empty() && forwardIcon->action()->isEnabled() )
            forwardIcon->action()->setEnabled( false );
    }
}

void
WikipediaAppletPrivate::_gotoAlbum()
{
    Q_Q( WikipediaApplet );
    q->dataEngine( "amarok-wikipedia" )->query( "wikipedia:AMAROK_TOKEN:goto:AMAROK_TOKEN:album" );
}

void
WikipediaAppletPrivate::_gotoArtist()
{
    Q_Q( WikipediaApplet );
    q->dataEngine( "amarok-wikipedia" )->query( "wikipedia:AMAROK_TOKEN:goto:AMAROK_TOKEN:artist" );
}

void
WikipediaAppletPrivate::_gotoTrack()
{
    Q_Q( WikipediaApplet );
    q->dataEngine( "amarok-wikipedia" )->query( "wikipedia:AMAROK_TOKEN:goto:AMAROK_TOKEN:track" );
}

void
WikipediaAppletPrivate::_linkClicked( const QUrl &url )
{
    Q_Q( WikipediaApplet );
    debug() << "linkClicked" << url;
    if( url.toString().contains( "wikipedia.org/" ) )
    {
        q->dataEngine( "amarok-wikipedia" )->query( QString( "wikipedia:AMAROK_TOKEN:get:AMAROK_TOKEN:" ) + url.toString() );
        if( backwardIcon->action() && !backwardIcon->action()->isEnabled() )
            backwardIcon->action()->setEnabled( true );

        historyForward.clear();
        if( forwardIcon->action() && forwardIcon->action()->isEnabled() )
            forwardIcon->action()->setEnabled( false );
    }
    else
        QDesktopServices::openUrl( url.toString() );
}

void
WikipediaAppletPrivate::_paletteChanged( const QPalette &palette )
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

        const QByteArray &css = contents.toLatin1();
        qint64 written = writeStyleSheet( css );
        if( written != -1 )
        {
            QUrl cssUrl( QString("data:text/css;charset=utf-8;base64,") + css.toBase64() );
            //NOTE  We give it encoded on a base64
            // as it is currently broken on QtWebkit (see https://bugs.webkit.org/show_bug.cgi?id=34884 )
            webView->mainFrame()->page()->settings()->setUserStyleSheetUrl( cssUrl );
        }
    }
    file.close();
}

void
WikipediaAppletPrivate::_reloadWikipedia()
{
    DEBUG_BLOCK
    Q_Q( WikipediaApplet );
    q->dataEngine( "amarok-wikipedia" )->query( "wikipedia:AMAROK_TOKEN:reload" );
}

void
WikipediaAppletPrivate::_switchLang()
{
    DEBUG_BLOCK
    Q_Q( WikipediaApplet );
    q->showConfigurationInterface();
}

void
WikipediaAppletPrivate::_switchToLang( const QString &lang )
{
    DEBUG_BLOCK
    Q_Q( WikipediaApplet );
    // TODO change this b/c it's BAAADDD !!!
    if (lang == i18nc("automatic language selection", "Automatic") )
        preferredLang = "aut";

    else if (lang == i18n("English") )
        preferredLang = "en";

    else if (lang == i18n("French") )
        preferredLang = "fr";

    else if (lang == i18n("German") )
        preferredLang = "de";

    q->dataEngine( "amarok-wikipedia" )->query( QString( "wikipedia:AMAROK_TOKEN:lang:AMAROK_TOKEN:" ) + preferredLang );

    KConfigGroup config = Amarok::config("Wikipedia Applet");
    config.writeEntry( "PreferredLang", preferredLang );
    q->dataEngine( "amarok-wikipedia" )->query( QString( "wikipedia:AMAROK_TOKEN:lang:AMAROK_TOKEN:" ) + preferredLang );
}

WikipediaApplet::WikipediaApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , d_ptr( new WikipediaAppletPrivate( this ) )
{
    setHasConfigurationInterface( true );
    setBackgroundHints( Plasma::Applet::NoBackground );
}

WikipediaApplet::~WikipediaApplet()
{
    Q_D( WikipediaApplet );
    delete d->webView;
    delete d->css;
    delete d_ptr;
}

void
WikipediaApplet::init()
{
    // Call the base implementation.
    Context::Applet::init();

    Q_D( WikipediaApplet );
    d->wikipediaLabel = new TextScrollingWidget( this );

    d->webView = new Plasma::WebView( this );
    d->webView->setAttribute( Qt::WA_NoSystemBackground );

    // ask for all the CV height
    resize( 500, -1 );

    d->_paletteChanged( App::instance()->palette() );
    connect( The::paletteHandler(), SIGNAL(newPalette(QPalette)), SLOT(_paletteChanged(QPalette)) );
    d->webView->page()->setLinkDelegationPolicy ( QWebPage::DelegateAllLinks );
    connect( d->webView->page(), SIGNAL(linkClicked(QUrl)), SLOT(_linkClicked(QUrl)) );

    // make transparent so we can use qpainter translucency to draw the  background
    QPalette palette = d->webView->palette();
    palette.setBrush(QPalette::Base, Qt::transparent);
    d->webView->page()->setPalette(palette);
    d->webView->setAttribute(Qt::WA_OpaquePaintEvent, false);

    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    d->wikipediaLabel->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    d->wikipediaLabel->setFont( labelFont );
    d->wikipediaLabel->setScrollingText( i18n( "Wikipedia" ) );

    QAction* backwardAction = new QAction( this );
    backwardAction->setIcon( KIcon( "go-previous" ) );
    backwardAction->setEnabled( false );
    backwardAction->setText( i18n( "Back" ) );
    d->backwardIcon = addAction( backwardAction );
    connect( d->backwardIcon, SIGNAL(clicked()), this, SLOT(_goBackward()) );

    QAction* forwardAction = new QAction( this );
    forwardAction->setIcon( KIcon( "go-next" ) );
    forwardAction->setEnabled( false );
    forwardAction->setText( i18n( "Forward" ) );
    d->forwardIcon = addAction( forwardAction );
    connect( d->forwardIcon, SIGNAL(clicked()), this, SLOT(_goForward()) );

    QAction* artistAction = new QAction( this );
    artistAction->setIcon( KIcon( "filename-artist-amarok" ) );
    artistAction->setEnabled( false );
    artistAction->setText( i18n( "Artist" ) );
    d->artistIcon = addAction( artistAction );
    connect( d->artistIcon, SIGNAL(clicked()), this, SLOT(_gotoArtist()) );

    QAction* albumAction = new QAction( this );
    albumAction->setIcon( KIcon( "filename-album-amarok" ) );
    albumAction->setEnabled( false );
    albumAction->setText( i18n( "Album" ) );
    d->albumIcon = addAction( albumAction );
    connect( d->albumIcon, SIGNAL(clicked()), this, SLOT(_gotoAlbum()) );

    QAction* trackAction = new QAction( this );
    trackAction->setIcon( KIcon( "filename-title-amarok" ) );
    trackAction->setEnabled( false );
    trackAction->setText( i18n( "Track" ) );
    d->trackIcon = addAction( trackAction );
    connect( d->trackIcon, SIGNAL(clicked()), this, SLOT(_gotoTrack()) );

    QAction* langAction = new QAction( this );
    langAction->setIcon( KIcon( "preferences-system" ) );
    langAction->setEnabled( true );
    langAction->setText( i18n( "Settings" ) );
    d->settingsIcon = addAction( langAction );
    connect( d->settingsIcon, SIGNAL(clicked()), this, SLOT(_switchLang()) );

    QAction* reloadAction = new QAction( this );
    reloadAction->setIcon( KIcon( "view-refresh" ) );
    reloadAction->setEnabled( false );
    reloadAction->setText( i18n( "Reload" ) );
    d->reloadIcon = addAction( reloadAction );
    connect( d->reloadIcon, SIGNAL(clicked()), this, SLOT(_reloadWikipedia()) );

    d->_connectSource( "wikipedia" );
    connect( dataEngine( "amarok-wikipedia" ), SIGNAL(sourceAdded(QString)), SLOT(_connectSource(QString)) );

    updateConstraints();

    // Read config and inform the engine.
    KConfigGroup config = Amarok::config("Wikipedia Applet");
    d->preferredLang = config.readEntry( "PreferredLang", "aut" );
    dataEngine( "amarok-wikipedia" )->query( QString( "wikipedia:AMAROK_TOKEN:lang:AMAROK_TOKEN:" ) + d->preferredLang );
}

void
WikipediaApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints );
    Q_D( WikipediaApplet );

    prepareGeometryChange();
    const float textWidth = d->wikipediaLabel->boundingRect().width();
    const float offsetX =  ( boundingRect().width() - textWidth ) / 2;

    const qreal widmax = boundingRect().width() - 4 * standardPadding();
    const QRectF rect( ( boundingRect().width() - widmax ) / 2, 0 , widmax, 15 );

    d->wikipediaLabel->setScrollingText( i18n( "Wikipedia" ) );
    d->wikipediaLabel->setPos( offsetX, standardPadding() + 2 );

    d->webView->setPos( standardPadding(), d->wikipediaLabel->pos().y() + d->wikipediaLabel->boundingRect().height() + standardPadding() );
    d->webView->resize( boundingRect().width() - 2 * standardPadding(), boundingRect().height() - d->webView->pos().y() - standardPadding() );

    // Icon positionning
    float iconWidth = d->backwardIcon->size().width();
    d->backwardIcon->setPos( size().width() - 7 * iconWidth - 6 * standardPadding(), standardPadding() );
    d->forwardIcon->setPos( size().width() - 6 * iconWidth - 6 * standardPadding(), standardPadding() );

    d->artistIcon->setPos( size().width() - 5 * iconWidth - 4 * standardPadding(), standardPadding() );
    d->albumIcon->setPos( size().width() - 4 * iconWidth - 4 * standardPadding(), standardPadding() );
    d->trackIcon->setPos( size().width() - 3 * iconWidth - 4 * standardPadding(), standardPadding() );

    d->reloadIcon->setPos( size().width() - 2 * iconWidth - 2 * standardPadding(), standardPadding() );
    d->settingsIcon->setPos( size().width() - iconWidth - standardPadding(), standardPadding() );
    update();
}

bool
WikipediaApplet::hasHeightForWidth() const
{
    return true;
}

qreal
WikipediaApplet::heightForWidth( qreal width ) const
{
    Q_D( const WikipediaApplet );
    return width * d->aspectRatio;
}

void
WikipediaApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    Q_UNUSED( name )
    Q_D( WikipediaApplet );

    if( data.size() == 0 )
        return;

    if( data.contains("busy") )
    {
        d->webView->hide();
        if( canAnimate() )
            setBusy( true );
        return;
    }
    else
    {
        d->webView->show();
        setBusy( false );
    }

    if( data.contains( "page" ) )
    {
        if ( d->current.page == data[ "page" ].toString() && !d->gotMessage)
            return;

        // save last page, useful when you are reading but the song changes
        if ( !d->current.page.isEmpty() )
        {
            d->historyBack.push_front( d->current );
            while ( d->historyBack.size() > 20 )
                d->historyBack.pop_back();

            if ( d->backwardIcon->action() && !d->backwardIcon->action()->isEnabled() )
                d->backwardIcon->action()->setEnabled( true );

        }
        d->current.page = data[ "page" ].toString();
        d->current.url = KUrl( data[ "url" ].toString() );
        d->webView->setHtml( d->current.page, d->current.url );
        d->gotMessage = false;
        d->historyForward.clear();

        if ( d->forwardIcon->action() && d->forwardIcon->action()->isEnabled() )
            d->forwardIcon->action()->setEnabled( false );
    }

    if( data.contains( "message" ) )
    {
        d->webView->setHtml( data[ "message" ].toString(), KUrl( QString() ) ); // set data
        d->gotMessage = true; // we have a message and don't want to save it in history
    }

    if( d->reloadIcon->action() && !d->reloadIcon->action()->isEnabled() )
        d->reloadIcon->action()->setEnabled( true );

    if( d->artistIcon->action() && !d->artistIcon->action()->isEnabled() )
        d->artistIcon->action()->setEnabled( true );

    if( d->albumIcon->action() && !d->albumIcon->action()->isEnabled() )
        d->albumIcon->action()->setEnabled( true );

    if( d->trackIcon->action() && !d->trackIcon->action()->isEnabled() )
        d->trackIcon->action()->setEnabled( true );
}

void
WikipediaApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option )
    Q_UNUSED( contentsRect )
    Q_D( WikipediaApplet );

    p->setRenderHint( QPainter::Antialiasing );

    addGradientToAppletBackground( p );

    // draw rounded rect around title
    drawRoundedRectAroundText( p, d->wikipediaLabel );

    //draw background of wiki text
    QSizeF wikiSize( d->webView->page()->viewportSize() );
    QRectF wikiRect( d->webView->pos(), wikiSize );

    p->save();
    QPainterPath round;
    round.addRoundedRect( wikiRect, 5, 5 );
    p->fillPath( round , The::paletteHandler()->backgroundColor() );
    p->restore();
}

void
WikipediaApplet::createConfigurationInterface( KConfigDialog *parent )
{
    Q_D( WikipediaApplet );
    KConfigGroup configuration = config();
    QWidget *settings = new QWidget;
    d->ui_Settings.setupUi( settings );

    // TODO bad, it's done manually ...
    if ( d->preferredLang == "aut" )
        d->ui_Settings.comboBox->setCurrentIndex( 0 );
    else if ( d->preferredLang == "en" )
        d->ui_Settings.comboBox->setCurrentIndex( 1 );
    else if ( d->preferredLang == "fr" )
        d->ui_Settings.comboBox->setCurrentIndex( 2 );
    else if ( d->preferredLang == "de" )
        d->ui_Settings.comboBox->setCurrentIndex( 3 );

    parent->addPage( settings, i18n( "Wikipedia Settings" ), "preferences-system");
    connect( d->ui_Settings.comboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(_switchToLang(QString)) );
}

#include "WikipediaApplet.moc"

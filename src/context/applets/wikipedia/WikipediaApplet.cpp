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
#include "WikipediaApplet_p.h"
#include "WikipediaApplet_p.moc"

#include "core/support/Amarok.h"
#include "App.h"
#include "core/support/Debug.h"
#include "PaletteHandler.h"
#include "widgets/TextScrollingWidget.h"

#include <KConfigDialog>
#include <KGlobalSettings>
#include <KSaveFile>
#include <KStandardDirs>
#include <KTemporaryFile>

#include <Plasma/DataContainer>
#include <Plasma/IconWidget>
#include <Plasma/Theme>

#include <QAction>
#include <QDesktopServices>
#include <QGraphicsLinearLayout>
#include <QGraphicsView>
#include <QListWidget>
#include <QPainter>
#include <QProgressBar>
#include <QTextStream>
#include <QWebPage>
#include <QXmlStreamReader>

void
WikipediaAppletPrivate::parseWikiLangXml( const QByteArray &data )
{
    QXmlStreamReader xml( data );
    while( !xml.atEnd() && !xml.hasError() )
    {
        xml.readNext();
        if( xml.isStartElement() && xml.name() == "iw" )
        {
            const QXmlStreamAttributes &a = xml.attributes();
            if( a.hasAttribute("prefix") && a.hasAttribute("language") && a.hasAttribute("url") )
            {
                const QString &prefix = a.value("prefix").toString();
                const QString &language = a.value("language").toString();
                const QString &display = QString( "[%1] %2" ).arg( prefix ).arg( language );
                QListWidgetItem *item = new QListWidgetItem( display, 0 );
                // The urlPrefix is the lang code infront of the wikipedia host
                // url. It is mostly the same as the "prefix" attribute but in
                // some weird cases they differ, so we can't just use "prefix".
                QString urlPrefix = QUrl( a.value("url").toString() ).host().remove(".wikipedia.org");
                item->setData( PrefixRole, prefix );
                item->setData( UrlPrefixRole, urlPrefix );
                item->setData( LanguageStringRole, language );
                ui.langSelector->availableListWidget()->addItem( item );
            }
        }
    }
}

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
WikipediaAppletPrivate::scheduleEngineUpdate()
{
    Q_Q( WikipediaApplet );
    q->dataEngine( "amarok-wikipedia" )->query( "update" );
}

void
WikipediaAppletPrivate::updateNavigationIcons()
{
    forwardIcon->action()->setEnabled( !historyForward.isEmpty() );
    backwardIcon->action()->setEnabled( !historyBack.isEmpty() );
}

void
WikipediaAppletPrivate::_goBackward()
{
    DEBUG_BLOCK
    if( !historyBack.empty() )
    {
        historyForward.push( currentUrl );
        currentUrl = historyBack.pop();
        isBackwardHistory = true;
        dataContainer->removeAllData();
        dataContainer->setData( "clickUrl", currentUrl );
        scheduleEngineUpdate();
        updateNavigationIcons();
    }
}

void
WikipediaAppletPrivate::_goForward()
{
    DEBUG_BLOCK
    if( !historyForward.empty() )
    {
        historyBack.push( currentUrl );
        currentUrl = historyForward.pop();
        isForwardHistory = true;
        dataContainer->removeAllData();
        dataContainer->setData( "clickUrl", currentUrl );
        scheduleEngineUpdate();
        updateNavigationIcons();
    }
}

void
WikipediaAppletPrivate::_gotoAlbum()
{
    dataContainer->setData( "goto", "album" );
    scheduleEngineUpdate();
}

void
WikipediaAppletPrivate::_gotoArtist()
{
    dataContainer->setData( "goto", "artist" );
    scheduleEngineUpdate();
}

void
WikipediaAppletPrivate::_gotoTrack()
{
    dataContainer->setData( "goto", "track" );
    scheduleEngineUpdate();
}

void
WikipediaAppletPrivate::_jsWindowObjectCleared()
{
    Q_Q( WikipediaApplet );
    webView->page()->mainFrame()->addToJavaScriptWindowObject( "mWebPage", q );
}

void
WikipediaAppletPrivate::_linkClicked( const QUrl &url )
{
    Q_Q( WikipediaApplet );
    if( url.host().contains( "wikipedia.org" ) )
    {
        q->setBusy( true );
        dataContainer->setData( "clickUrl", url );
        scheduleEngineUpdate();
        updateNavigationIcons();
    }
    else
        QDesktopServices::openUrl( url.toString() );
}

void
WikipediaAppletPrivate::_loadSettings()
{
    QStringList list;
    QListWidget *listWidget = ui.langSelector->selectedListWidget();
    for( int i = 0, count = listWidget->count(); i < count; ++i )
    {
        QListWidgetItem *item = listWidget->item( i );
        const QString &prefix = item->data( PrefixRole ).toString();
        const QString &urlPrefix = item->data( UrlPrefixRole ).toString();
        QString concat = QString("%1:%2").arg( prefix ).arg( urlPrefix );
        list << (prefix == urlPrefix ? prefix : concat);
    }
    langList = list;
    Amarok::config("Wikipedia Applet").writeEntry( "PreferredLang", list );
    dataContainer->setData( "lang", langList );
    scheduleEngineUpdate();
}

void
WikipediaAppletPrivate::_paletteChanged( const QPalette &palette )
{
    // read css, replace color placeholders, write to file, load into page
    QFile file( KStandardDirs::locate("data", "amarok/data/WikipediaCustomStyle.css" ) );
    if( file.open(QIODevice::ReadOnly | QIODevice::Text) )
    {
        // transparent background
        QPalette newPalette( palette );
        newPalette.setBrush( QPalette::Base, QColor::fromRgbF(0, 0, 0, 0) );
        webView->page()->setPalette( newPalette );

        QString contents = QString( file.readAll() );
        contents.replace( "/*{text_color}*/", palette.text().color().name() );
        contents.replace( "/*{link_color}*/", palette.link().color().name() );
        contents.replace( "/*{link_hover_color}*/", palette.linkVisited().color().name() );

        const QString abg = The::paletteHandler()->alternateBackgroundColor().name();
        contents.replace( "/*{shaded_text_background_color}*/", abg );
        contents.replace( "/*{table_background_color}*/", abg );
        contents.replace( "/*{headings_background_color}*/", abg );

        const QString hiColor = The::paletteHandler()->highlightColor().name();
        contents.replace( "/*{border_color}*/", hiColor );

        const QString atbg = palette.highlight().color().name();
        contents.replace( "/*{alternate_table_background_color}*/", atbg );

        const QByteArray &css = contents.toLatin1();
        qint64 written = writeStyleSheet( css );
        if( written != -1 )
        {
            QUrl cssUrl( QString("data:text/css;charset=utf-8;base64,") + css.toBase64() );
            //NOTE  We give it encoded on a base64
            // as it is currently broken on QtWebkit (see https://bugs.webkit.org/show_bug.cgi?id=34884 )
            webView->settings()->setUserStyleSheetUrl( cssUrl );
        }
    }
}

void
WikipediaAppletPrivate::_reloadWikipedia()
{
    DEBUG_BLOCK
    dataContainer->setData( "reload", true );
    scheduleEngineUpdate();
}

void
WikipediaAppletPrivate::_updateWebFonts()
{
    Q_Q( WikipediaApplet );
    if( !q->view() )
        return;
    qreal ratio = q->view()->logicalDpiY() / 72.0;
    qreal fixedFontSize = KGlobalSettings::fixedFont().pointSize() * ratio;
    qreal generalFontSize = KGlobalSettings::generalFont().pointSize() * ratio;
    qreal minimumFontSize = KGlobalSettings::smallestReadableFont().pointSize() * ratio;
    QWebSettings *webSettings = webView->page()->settings();
    webSettings->setFontSize( QWebSettings::DefaultFixedFontSize, qRound(fixedFontSize) );
    webSettings->setFontSize( QWebSettings::DefaultFontSize, qRound(generalFontSize) );
    webSettings->setFontSize( QWebSettings::MinimumFontSize, qRound(minimumFontSize) );
}

void
WikipediaAppletPrivate::_getLangMapProgress( qint64 received, qint64 total )
{
    ui.progressBar->setValue( 100.0 * qreal(received) / total );
}

void
WikipediaAppletPrivate::_getLangMapFinished( const KUrl &url, QByteArray data,
                                             NetworkAccessManagerProxy::Error e )
{
    Q_UNUSED( url )
    ui.downloadButton->setEnabled( true );
    ui.progressBar->setEnabled( false );

    if( e.code != QNetworkReply::NoError )
    {
        debug() << "Downloading Wikipedia supported languages failed:" << e.description;
        return;
    }

    QListWidget *availableListWidget = ui.langSelector->availableListWidget();
    availableListWidget->clear();
    parseWikiLangXml( data );
    ui.langSelector->setButtonsEnabled();
    QString buttonText = ( availableListWidget->count() > 0 )
                       ? i18n( "Update Supported Languages" )
                       : i18n( "Get Supported Languages" );
    ui.downloadButton->setText( buttonText );

    QListWidget *selectedListWidget = ui.langSelector->selectedListWidget();
    QList<QListWidgetItem*> selectedListItems = selectedListWidget->findItems( QChar('*'), Qt::MatchWildcard );
    for( int i = 0, count = selectedListItems.count(); i < count; ++i )
    {
        QListWidgetItem *item = selectedListItems.at( i );
        int rowAtSelectedList = selectedListWidget->row( item );
        item = selectedListWidget->takeItem( rowAtSelectedList );
        const QString &prefix = item->data( PrefixRole ).toString();
        QList<QListWidgetItem*> foundItems = availableListWidget->findItems( QString("[%1]").arg( prefix ),
                                                                             Qt::MatchStartsWith );
        // should only have found one item if any
        if( !foundItems.isEmpty() )
        {
            item = foundItems.first();
            int rowAtAvailableList = ui.langSelector->availableListWidget()->row( item );
            item = availableListWidget->takeItem( rowAtAvailableList );
            selectedListWidget->addItem( item );
        }
    }

    KSaveFile saveFile;
    saveFile.setFileName( Amarok::saveLocation() + "wikipedia_languages.xml" );
    if( saveFile.open() )
    {
        debug() << "Saving" << saveFile.fileName();
        QTextStream stream( &saveFile );
        stream << data;
        stream.flush();
        saveFile.finalize();
    }
    else
    {
        debug() << "Failed to saving Wikipedia languages file";
    }
}

void
WikipediaAppletPrivate::_getLangMap()
{
    Q_Q( WikipediaApplet );
    ui.downloadButton->setEnabled( false );
    ui.progressBar->setEnabled( true );
    ui.progressBar->setMaximum( 100 );
    ui.progressBar->setValue( 0 );

    KUrl url;
    url.setScheme( "http" );
    url.setHost( "en.wikipedia.org" );
    url.setPath( "/w/api.php" );
    url.addQueryItem( "action", "query" );
    url.addQueryItem( "meta", "siteinfo" );
    url.addQueryItem( "siprop", "interwikimap" );
    url.addQueryItem( "sifilteriw", "local" );
    url.addQueryItem( "format", "xml" );
    QNetworkReply *reply = The::networkAccessManager()->getData( url, q,
                           SLOT(_getLangMapFinished(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
    q->connect( reply, SIGNAL(downloadProgress(qint64,qint64)), q, SLOT(_getLangMapProgress(qint64,qint64)) );
}

void
WikipediaAppletPrivate::_configureLangSelector()
{
    DEBUG_BLOCK
    Q_Q( WikipediaApplet );

    QFile savedFile( Amarok::saveLocation() + "wikipedia_languages.xml" );
    if( savedFile.open(QIODevice::ReadOnly | QIODevice::Text) )
        parseWikiLangXml( savedFile.readAll() );
    savedFile.close();

    QListWidget *availableListWidget = ui.langSelector->availableListWidget();
    QString buttonText = ( availableListWidget->count() > 0 )
                       ? i18n( "Update Supported Languages" )
                       : i18n( "Get Supported Languages" );
    ui.downloadButton->setText( buttonText );

    for( int i = 0, count = langList.count(); i < count; ++i )
    {
        const QStringList &split = langList.at( i ).split( QLatin1Char(':') );
        const QString &prefix    = split.first();
        const QString &urlPrefix = (split.count() == 1) ? prefix : split.at( 1 );
        QList<QListWidgetItem*> foundItems = availableListWidget->findItems( QString("[%1]").arg( prefix ),
                                                                             Qt::MatchStartsWith );
        if( foundItems.isEmpty() )
        {
            QListWidgetItem *item = new QListWidgetItem( prefix, 0 );
            item->setData( WikipediaAppletPrivate::PrefixRole, prefix );
            item->setData( WikipediaAppletPrivate::UrlPrefixRole, urlPrefix );
            ui.langSelector->selectedListWidget()->addItem( item );
        }
        else // should only have found one item if any
        {
            QListWidgetItem *item = foundItems.first();
            int rowAtAvailableList = availableListWidget->row( item );
            item = availableListWidget->takeItem( rowAtAvailableList );
            ui.langSelector->selectedListWidget()->addItem( item );
        }
    }
    q->connect( ui.langSelector, SIGNAL(added(QListWidgetItem*)), q, SLOT(_langSelectorItemChanged(QListWidgetItem*)) );
    q->connect( ui.langSelector, SIGNAL(movedDown(QListWidgetItem*)), q, SLOT(_langSelectorItemChanged(QListWidgetItem*)) );
    q->connect( ui.langSelector, SIGNAL(movedUp(QListWidgetItem*)), q, SLOT(_langSelectorItemChanged(QListWidgetItem*)) );
    q->connect( ui.langSelector, SIGNAL(removed(QListWidgetItem*)), q, SLOT(_langSelectorItemChanged(QListWidgetItem*)) );
    q->connect( ui.langSelector->availableListWidget(), SIGNAL(itemClicked(QListWidgetItem*)), q, SLOT(_langSelectorItemChanged(QListWidgetItem*)) );
    q->connect( ui.langSelector->selectedListWidget(), SIGNAL(itemClicked(QListWidgetItem*)), q, SLOT(_langSelectorItemChanged(QListWidgetItem*)) );
}

void
WikipediaAppletPrivate::_pageLoadStarted()
{
    Q_Q( WikipediaApplet );
    QGraphicsProxyWidget *proxy = new QGraphicsProxyWidget;
    proxy->setWidget( new QProgressBar );
    QGraphicsLinearLayout *lo = static_cast<QGraphicsLinearLayout*>( q->layout() );
    lo->addItem( proxy );
    lo->activate();
    QObject::connect( webView, SIGNAL(loadProgress(int)), q, SLOT(_pageLoadProgress(int)) );
}

void
WikipediaAppletPrivate::_pageLoadProgress( int progress )
{
    Q_Q( WikipediaApplet );
    QGraphicsLinearLayout *lo = static_cast<QGraphicsLinearLayout*>( q->layout() );
    QGraphicsProxyWidget *proxy = static_cast<QGraphicsProxyWidget*>( lo->itemAt( lo->count() - 1 ) );
    QString kbytes = QString::number( webView->page()->totalBytes() / 1024 );
    QProgressBar *pbar = static_cast<QProgressBar*>( proxy->widget() );
    pbar->setFormat( QString( "%1kB : %p%" ).arg( kbytes ) );
    pbar->setValue( progress );
}

void
WikipediaAppletPrivate::_pageLoadFinished( bool ok )
{
    Q_UNUSED( ok )
    Q_Q( WikipediaApplet );
    QGraphicsLinearLayout *lo = static_cast<QGraphicsLinearLayout*>( q->layout() );
    QGraphicsProxyWidget *proxy = static_cast<QGraphicsProxyWidget*>( lo->itemAt( lo->count() - 1 ) );
    lo->removeItem( proxy );
    lo->activate();
    proxy->deleteLater();
}

void
WikipediaAppletPrivate::_searchLineEditTextEdited( const QString &text )
{
    webView->page()->findText( QString(), QWebPage::HighlightAllOccurrences ); // clears preivous highlights
    webView->page()->findText( text, QWebPage::FindWrapsAroundDocument | QWebPage::HighlightAllOccurrences );
}

void
WikipediaAppletPrivate::_searchLineEditReturnPressed()
{
    const QString &text = webView->lineEdit()->text();
    webView->page()->findText( text, QWebPage::FindWrapsAroundDocument );
}

void
WikipediaAppletPrivate::_langSelectorItemChanged( QListWidgetItem *item )
{
    Q_UNUSED( item )
    ui.langSelector->setButtonsEnabled();
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

    // ask for all the CV height
    resize( 500, -1 );

    d->wikipediaLabel = new TextScrollingWidget( this );
    d->webView = new WikipediaWebView( this );
    d->webView->page()->mainFrame()->setScrollBarPolicy( Qt::Horizontal, Qt::ScrollBarAlwaysOff );
    d->webView->page()->mainFrame()->addToJavaScriptWindowObject( "mWebPage", this );
    d->webView->page()->setNetworkAccessManager( The::networkAccessManager() );
    d->webView->page()->setLinkDelegationPolicy ( QWebPage::DelegateAllLinks );
    d->webView->page()->settings()->setAttribute( QWebSettings::PrivateBrowsingEnabled, true );
    QWebSettings::globalSettings()->setAttribute( QWebSettings::DnsPrefetchEnabled, true );
    d->webView->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    d->webView->setAttribute( Qt::WA_OpaquePaintEvent, true );
    d->_updateWebFonts();
    connect( KGlobalSettings::self(), SIGNAL(appearanceChanged()), SLOT(_updateWebFonts()) );

    d->_paletteChanged( App::instance()->palette() );
    connect( The::paletteHandler(), SIGNAL(newPalette(QPalette)), SLOT(_paletteChanged(QPalette)) );
    connect( d->webView->page(), SIGNAL(linkClicked(QUrl)), SLOT(_linkClicked(QUrl)) );
    connect( d->webView->page(), SIGNAL(loadStarted()), SLOT(_pageLoadStarted()) );
    connect( d->webView->page(), SIGNAL(loadFinished(bool)), SLOT(_pageLoadFinished(bool)) );
    connect( d->webView->page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), SLOT(_jsWindowObjectCleared()) );
    connect( d->webView->lineEdit(), SIGNAL(textChanged(QString)), SLOT(_searchLineEditTextEdited(QString)) );
    connect( d->webView->lineEdit(), SIGNAL(returnPressed()), SLOT(_searchLineEditReturnPressed()) );

    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    d->wikipediaLabel->setFont( labelFont );
    d->wikipediaLabel->setScrollingText( i18n( "Wikipedia" ) );
    d->wikipediaLabel->setDrawBackground( true );

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
    connect( d->settingsIcon, SIGNAL(clicked()), this, SLOT(showConfigurationInterface()) );

    QAction* reloadAction = new QAction( this );
    reloadAction->setIcon( KIcon( "view-refresh" ) );
    reloadAction->setEnabled( false );
    reloadAction->setText( i18n( "Reload" ) );
    d->reloadIcon = addAction( reloadAction );
    connect( d->reloadIcon, SIGNAL(clicked()), this, SLOT(_reloadWikipedia()) );

    QGraphicsLinearLayout *headerLayout = new QGraphicsLinearLayout( Qt::Horizontal );
    headerLayout->addItem( d->backwardIcon );
    headerLayout->addItem( d->forwardIcon );
    headerLayout->addItem( d->reloadIcon );
    headerLayout->addItem( d->wikipediaLabel );
    headerLayout->addItem( d->artistIcon );
    headerLayout->addItem( d->albumIcon );
    headerLayout->addItem( d->trackIcon );
    headerLayout->addItem( d->settingsIcon );
    headerLayout->setContentsMargins( 0, 4, 0, 4 );

    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout( Qt::Vertical );
    layout->setSpacing( 2 );
    layout->addItem( headerLayout );
    layout->addItem( d->webView );
    setLayout( layout );

    dataEngine( "amarok-wikipedia" )->connectSource( "wikipedia", this );
    d->dataContainer = dataEngine( "amarok-wikipedia" )->containerForSource( "wikipedia" );

    // Read config and inform the engine.
    d->langList = Amarok::config("Wikipedia Applet").readEntry( "PreferredLang", QStringList() << "en" );
    d->dataContainer->setData( "lang", d->langList );
    d->scheduleEngineUpdate();

    updateConstraints();
    update();
}

void
WikipediaApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints );
    Q_D( WikipediaApplet );
    d->wikipediaLabel->setScrollingText( d->wikipediaLabel->text() );
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
WikipediaApplet::dataUpdated( const QString &source, const Plasma::DataEngine::Data &data )
{
    Q_UNUSED( source )
    Q_D( WikipediaApplet );

    if( data.isEmpty() )
        return;

    if( data.contains( "busy" ) )
    {
        if( canAnimate() && data["busy"].toBool() )
        {
            d->webView->hide();
            setBusy( true );
        }
        return;
    }
    else
    {
        d->webView->show();
        setBusy( false );
    }

    if( data.contains( "message" ) )
    {
        // messages have higher priority than pages
        const QString &message = data.value( "message" ).toString();
        if( !message.isEmpty() )
        {
            d->webView->setHtml( data[ "message" ].toString(), QUrl() ); // set data
            d->wikipediaLabel->setScrollingText( i18n( "Wikipedia" ) );
            d->dataContainer->removeAllData();
        }
    }
    else if( data.contains( "page" ) )
    {
        if( data.contains( "url" ) && !data.value( "url" ).toUrl().isEmpty() )
        {
            const QUrl &url = data.value( "url" ).toUrl();
            if( !d->isForwardHistory && !d->isBackwardHistory && !d->currentUrl.isEmpty() )
            {
                // save last page, useful when you are reading but the song changes
                d->historyBack.push( d->currentUrl );
                d->historyForward.clear();
            }
            d->currentUrl = url;
            d->webView->setHtml( data[ "page" ].toString(), url );
            d->wikipediaLabel->setScrollingText( d->webView->page()->currentFrame()->title() );
            d->updateNavigationIcons();
            d->isBackwardHistory = false;
            d->isForwardHistory = false;
            d->dataContainer->removeAllData();
        }
    }
    else
    {
        d->wikipediaLabel->setScrollingText( i18n( "Wikipedia" ) );
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
WikipediaApplet::loadWikipediaUrl( const QString &url )
{
    Q_D( WikipediaApplet );
    d->_linkClicked( QUrl(url) );
}

void
WikipediaApplet::createConfigurationInterface( KConfigDialog *parent )
{
    Q_D( WikipediaApplet );
    KConfigGroup configuration = config();
    QWidget *settings = new QWidget;
    d->ui.setupUi( settings );
    d->ui.downloadButton->setGuiItem( KStandardGuiItem::find() );
    d->ui.langSelector->availableListWidget()->setAlternatingRowColors( true );
    d->ui.langSelector->selectedListWidget()->setAlternatingRowColors( true );
    d->ui.langSelector->availableListWidget()->setUniformItemSizes( true );
    d->ui.langSelector->selectedListWidget()->setUniformItemSizes( true );
    d->ui.progressBar->setEnabled( false );

    connect( d->ui.downloadButton, SIGNAL(clicked()), this, SLOT(_getLangMap()) );
    connect( parent, SIGNAL(applyClicked()), this, SLOT(_loadSettings()) );
    connect( parent, SIGNAL(okClicked()), this, SLOT(_loadSettings()) );

    parent->addPage( settings, i18n( "Wikipedia Language Settings" ), "applications-education-language");
    QTimer::singleShot( 0, this, SLOT(_configureLangSelector()) );
}

#include "WikipediaApplet.moc"

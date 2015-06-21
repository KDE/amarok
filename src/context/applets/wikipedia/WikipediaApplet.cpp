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

#include "App.h"
#include "core/support/Amarok.h"
#include "context/widgets/AppletHeader.h"
#include "core/support/Debug.h"
#include "PaletteHandler.h"

#include <KConfigDialog>
#include <KGlobalSettings>
#include <KSaveFile>
#include <KStandardDirs>
#include <KTemporaryFile>

#include <Plasma/DataContainer>
#include <Plasma/IconWidget>
#include <Plasma/Theme>

#include <QAction>
#include <QTimer>
#include <QDesktopServices>
#include <QGraphicsLinearLayout>
#include <QGraphicsView>
#include <QListWidget>
#include <QPainter>
#include <QProgressBar>
#include <QTextStream>
#include <QWebPage>
#include <QXmlStreamReader>
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

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
                const QString &display = QString( "[%1] %2" ).arg( prefix, language );
                QListWidgetItem *item = new QListWidgetItem( display, 0 );
                // The urlPrefix is the lang code infront of the wikipedia host
                // url. It is mostly the same as the "prefix" attribute but in
                // some weird cases they differ, so we can't just use "prefix".
                QString urlPrefix = QUrl( a.value("url").toString() ).host().remove(".wikipedia.org");
                item->setData( PrefixRole, prefix );
                item->setData( UrlPrefixRole, urlPrefix );
                item->setData( LanguageStringRole, language );
                languageSettingsUi.langSelector->availableListWidget()->addItem( item );
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
WikipediaAppletPrivate::setUrl( const QUrl &url )
{
    webView->settings()->resetFontSize( QWebSettings::MinimumFontSize );
    webView->settings()->resetFontSize( QWebSettings::MinimumLogicalFontSize );
    webView->settings()->resetFontSize( QWebSettings::DefaultFontSize );
    webView->settings()->resetFontSize( QWebSettings::DefaultFixedFontSize );
    webView->settings()->resetFontFamily( QWebSettings::StandardFont );
    webView->setUrl( url );
    currentUrl = url;
    dataContainer->removeAllData();
}

void
WikipediaAppletPrivate::pushUrlHistory( const QUrl &url )
{
    if( !isForwardHistory && !isBackwardHistory && !url.isEmpty() )
    {
        if( historyBack.isEmpty() || (!historyBack.isEmpty() && (url != historyBack.top())) )
            historyBack.push( url );
        historyForward.clear();
    }
    isBackwardHistory = false;
    isForwardHistory = false;
    updateNavigationIcons();
}

void
WikipediaAppletPrivate::updateNavigationIcons()
{
    forwardIcon->action()->setEnabled( !historyForward.isEmpty() );
    backwardIcon->action()->setEnabled( !historyBack.isEmpty() );
}

void
WikipediaAppletPrivate::_titleChanged( const QString &title )
{
    Q_Q( WikipediaApplet );
    q->setHeaderText( title );
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
WikipediaAppletPrivate::_gotoComposer()
{
    dataContainer->setData( "goto", "composer" );
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
    DEBUG_BLOCK
    Q_Q( WikipediaApplet );
    if( url.host().contains( "wikipedia.org" ) )
    {
        isBackwardHistory = false;
        isForwardHistory = false;
        pushUrlHistory( currentUrl );
        if( useMobileWikipedia )
        {
            setUrl( url );
            return;
        }
        q->setBusy( true );
        dataContainer->setData( "clickUrl", url );
        scheduleEngineUpdate();
    }
    else
        QDesktopServices::openUrl( url.toString() );
}

void
WikipediaAppletPrivate::_loadSettings()
{
    QStringList list;
    QListWidget *listWidget = languageSettingsUi.langSelector->selectedListWidget();
    for( int i = 0, count = listWidget->count(); i < count; ++i )
    {
        QListWidgetItem *item = listWidget->item( i );
        const QString &prefix = item->data( PrefixRole ).toString();
        const QString &urlPrefix = item->data( UrlPrefixRole ).toString();
        QString concat = QString("%1:%2").arg( prefix, urlPrefix );
        list << (prefix == urlPrefix ? prefix : concat);
    }
    langList = list;
    useMobileWikipedia = (generalSettingsUi.mobileCheckBox->checkState() == Qt::Checked);
    useSSL = (generalSettingsUi.sslCheckBox->checkState() == Qt::Checked);
    Amarok::config("Wikipedia Applet").writeEntry( "PreferredLang", list );
    Amarok::config("Wikipedia Applet").writeEntry( "UseMobile", useMobileWikipedia );
    Amarok::config( "Wikipedia Applet" ).writeEntry( "UseSSL", useSSL );
    _paletteChanged( App::instance()->palette() );
    dataContainer->setData( "lang", langList );
    dataContainer->setData( "mobile", useMobileWikipedia );
    dataContainer->setData( "ssl", useSSL );
    scheduleEngineUpdate();
}

void
WikipediaAppletPrivate::_paletteChanged( const QPalette &palette )
{
    if( useMobileWikipedia )
    {
        webView->settings()->setUserStyleSheetUrl( QUrl() );
        return;
    }

    // read css, replace color placeholders, write to file, load into page
    QFile file( KStandardDirs::locate("data", "amarok/data/WikipediaCustomStyle.css" ) );
    if( file.open(QIODevice::ReadOnly | QIODevice::Text) )
    {
        // transparent background
        QPalette newPalette( palette );
        newPalette.setBrush( QPalette::Base, Qt::transparent );
        webView->page()->setPalette( newPalette );
        webView->setPalette( newPalette );
        webView->setAttribute( Qt::WA_OpaquePaintEvent, false );

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
    if( useMobileWikipedia )
    {
        webView->reload();
        return;
    }
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
    webSettings->setFontFamily( QWebSettings::StandardFont, KGlobalSettings::generalFont().family() );
}

void
WikipediaAppletPrivate::_getLangMapProgress( qint64 received, qint64 total )
{
    languageSettingsUi.progressBar->setValue( 100.0 * qreal(received) / total );
}

void
WikipediaAppletPrivate::_getLangMapFinished( const QUrl &url, QByteArray data,
                                             NetworkAccessManagerProxy::Error e )
{
    Q_UNUSED( url )
    languageSettingsUi.downloadButton->setEnabled( true );
    languageSettingsUi.progressBar->setEnabled( false );

    if( e.code != QNetworkReply::NoError )
    {
        debug() << "Downloading Wikipedia supported languages failed:" << e.description;
        return;
    }

    QListWidget *availableListWidget = languageSettingsUi.langSelector->availableListWidget();
    availableListWidget->clear();
    parseWikiLangXml( data );
    languageSettingsUi.langSelector->setButtonsEnabled();
    QString buttonText = ( availableListWidget->count() > 0 )
                       ? i18n( "Update Supported Languages" )
                       : i18n( "Get Supported Languages" );
    languageSettingsUi.downloadButton->setText( buttonText );

    QListWidget *selectedListWidget = languageSettingsUi.langSelector->selectedListWidget();
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
            int rowAtAvailableList = languageSettingsUi.langSelector->availableListWidget()->row( item );
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
    languageSettingsUi.downloadButton->setEnabled( false );
    languageSettingsUi.progressBar->setEnabled( true );
    languageSettingsUi.progressBar->setMaximum( 100 );
    languageSettingsUi.progressBar->setValue( 0 );

    QUrl url;
    url.setScheme( "http" );
    url.setHost( "en.wikipedia.org" );
    url.setPath( "/w/api.php" );
    url.addQueryItem( "action", "query" );
    url.addQueryItem( "meta", "siteinfo" );
    url.addQueryItem( "siprop", "interwikimap" );
    url.addQueryItem( "sifilteriw", "local" );
    url.addQueryItem( "format", "xml" );
    QNetworkReply *reply = The::networkAccessManager()->getData( url, q,
                           SLOT(_getLangMapFinished(QUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
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

    QListWidget *availableListWidget = languageSettingsUi.langSelector->availableListWidget();
    QString buttonText = ( availableListWidget->count() > 0 )
                       ? i18n( "Update Supported Languages" )
                       : i18n( "Get Supported Languages" );
    languageSettingsUi.downloadButton->setText( buttonText );

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
            languageSettingsUi.langSelector->selectedListWidget()->addItem( item );
        }
        else // should only have found one item if any
        {
            QListWidgetItem *item = foundItems.first();
            int rowAtAvailableList = availableListWidget->row( item );
            item = availableListWidget->takeItem( rowAtAvailableList );
            languageSettingsUi.langSelector->selectedListWidget()->addItem( item );
        }
    }
    q->connect( languageSettingsUi.langSelector, SIGNAL(added(QListWidgetItem*)), q, SLOT(_langSelectorItemChanged(QListWidgetItem*)) );
    q->connect( languageSettingsUi.langSelector, SIGNAL(movedDown(QListWidgetItem*)), q, SLOT(_langSelectorItemChanged(QListWidgetItem*)) );
    q->connect( languageSettingsUi.langSelector, SIGNAL(movedUp(QListWidgetItem*)), q, SLOT(_langSelectorItemChanged(QListWidgetItem*)) );
    q->connect( languageSettingsUi.langSelector, SIGNAL(removed(QListWidgetItem*)), q, SLOT(_langSelectorItemChanged(QListWidgetItem*)) );
    q->connect( languageSettingsUi.langSelector->availableListWidget(), SIGNAL(itemClicked(QListWidgetItem*)), q, SLOT(_langSelectorItemChanged(QListWidgetItem*)) );
    q->connect( languageSettingsUi.langSelector->selectedListWidget(), SIGNAL(itemClicked(QListWidgetItem*)), q, SLOT(_langSelectorItemChanged(QListWidgetItem*)) );
}

void
WikipediaAppletPrivate::_pageLoadStarted()
{
    Q_Q( WikipediaApplet );

    // create a proxy widget for displaying the webview load status in form of a progress bar

    // if the proxyWidget still exists, re-use the existing object
    if ( proxyWidget )
        return;

    proxyWidget = new QGraphicsProxyWidget;
    proxyWidget->setWidget( new QProgressBar );

    // add proxy widget to layout
    QGraphicsLinearLayout *lo = static_cast<QGraphicsLinearLayout*>( q->layout() );
    lo->addItem( proxyWidget );
    lo->activate();
    QObject::connect( webView, SIGNAL(loadProgress(int)), q, SLOT(_pageLoadProgress(int)) );
}

void
WikipediaAppletPrivate::_pageLoadProgress( int progress )
{
    DEBUG_ASSERT(proxyWidget, return)

    const QString kbytes = QString::number( webView->page()->totalBytes() / 1024 );

    QProgressBar *pbar = qobject_cast<QProgressBar*>( proxyWidget->widget() );
    pbar->setFormat( QString( "%1kB : %p%" ).arg( kbytes ) );
    pbar->setValue( progress );
}

void
WikipediaAppletPrivate::_pageLoadFinished( bool ok )
{
    Q_UNUSED( ok )
    Q_Q( WikipediaApplet );

    // remove proxy widget from layout again, delete it
    QGraphicsLinearLayout *lo = static_cast<QGraphicsLinearLayout*>( q->layout() );
    lo->removeItem( proxyWidget );
    lo->activate();

    // disconnect (so that we don't get any further progress signalling) and delete widget
    QObject::disconnect( webView, SIGNAL(loadProgress(int)), q, SLOT(_pageLoadProgress(int)) );
    proxyWidget->deleteLater();
    proxyWidget = 0;
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
    languageSettingsUi.langSelector->setButtonsEnabled();
}

WikipediaApplet::WikipediaApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , d_ptr( new WikipediaAppletPrivate( this ) )
{
    setHasConfigurationInterface( true );
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
    DEBUG_BLOCK

    Context::Applet::init();

    Q_D( WikipediaApplet );

    d->webView = new WikipediaWebView( this );
    d->webView->page()->mainFrame()->setScrollBarPolicy( Qt::Horizontal, Qt::ScrollBarAlwaysOff );
    // d->webView->page()->mainFrame()->addToJavaScriptWindowObject( "mWebPage", this ); BUG:259075
    d->webView->page()->setNetworkAccessManager( The::networkAccessManager() );
    d->webView->page()->setLinkDelegationPolicy ( QWebPage::DelegateAllLinks );
    d->webView->page()->settings()->setAttribute( QWebSettings::PrivateBrowsingEnabled, true );
    QWebSettings::globalSettings()->setAttribute( QWebSettings::DnsPrefetchEnabled, true );
    d->webView->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    d->_updateWebFonts();
    connect( KGlobalSettings::self(), SIGNAL(appearanceChanged()), SLOT(_updateWebFonts()) );

    connect( The::paletteHandler(), SIGNAL(newPalette(QPalette)), SLOT(_paletteChanged(QPalette)) );
    connect( d->webView->page(), SIGNAL(linkClicked(QUrl)), SLOT(_linkClicked(QUrl)) );
    connect( d->webView->page(), SIGNAL(loadStarted()), SLOT(_pageLoadStarted()) );
    connect( d->webView->page(), SIGNAL(loadFinished(bool)), SLOT(_pageLoadFinished(bool)) );
    // connect( d->webView->page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), SLOT(_jsWindowObjectCleared()) ); BUG:259075
    connect( d->webView->lineEdit(), SIGNAL(textChanged(QString)), SLOT(_searchLineEditTextEdited(QString)) );
    connect( d->webView->lineEdit(), SIGNAL(returnPressed()), SLOT(_searchLineEditReturnPressed()) );
    connect( d->webView, SIGNAL(titleChanged(QString)), this, SLOT(_titleChanged(QString)) );

    enableHeader( true );
    setHeaderText( i18n( "Wikipedia" ) );

    setCollapseOffHeight( -1 );
    setCollapseHeight( m_header->height() );
    setMinimumHeight( collapseHeight() );
    setPreferredHeight( collapseHeight() );

    QAction* backwardAction = new QAction( this );
    backwardAction->setIcon( QIcon::fromTheme( "go-previous" ) );
    backwardAction->setEnabled( false );
    backwardAction->setText( i18n( "Back" ) );
    d->backwardIcon = addLeftHeaderAction( backwardAction );
    connect( d->backwardIcon, SIGNAL(clicked()), this, SLOT(_goBackward()) );

    QAction* forwardAction = new QAction( this );
    forwardAction->setIcon( QIcon::fromTheme( "go-next" ) );
    forwardAction->setEnabled( false );
    forwardAction->setText( i18n( "Forward" ) );
    d->forwardIcon = addLeftHeaderAction( forwardAction );
    connect( d->forwardIcon, SIGNAL(clicked()), this, SLOT(_goForward()) );

    QAction* reloadAction = new QAction( this );
    reloadAction->setIcon( QIcon::fromTheme( "view-refresh" ) );
    reloadAction->setText( i18n( "Reload" ) );
    d->reloadIcon = addLeftHeaderAction( reloadAction );
    connect( d->reloadIcon, SIGNAL(clicked()), this, SLOT(_reloadWikipedia()) );

    QAction* artistAction = new QAction( this );
    artistAction->setIcon( QIcon::fromTheme( "filename-artist-amarok" ) );
    artistAction->setText( i18n( "Artist" ) );
    d->artistIcon = addRightHeaderAction( artistAction );
    connect( d->artistIcon, SIGNAL(clicked()), this, SLOT(_gotoArtist()) );

    QAction* composerAction = new QAction( this );
    composerAction->setIcon( QIcon::fromTheme( "filename-composer-amarok" ) );
    composerAction->setText( i18n( "Composer" ) );
    d->composerIcon = addRightHeaderAction( composerAction );
    connect( d->composerIcon, SIGNAL(clicked()), this, SLOT(_gotoComposer()) );

    QAction* albumAction = new QAction( this );
    albumAction->setIcon( QIcon::fromTheme( "filename-album-amarok" ) );
    albumAction->setText( i18n( "Album" ) );
    d->albumIcon = addRightHeaderAction( albumAction );
    connect( d->albumIcon, SIGNAL(clicked()), this, SLOT(_gotoAlbum()) );

    QAction* trackAction = new QAction( this );
    trackAction->setIcon( QIcon::fromTheme( "filename-title-amarok" ) );
    trackAction->setText( i18n( "Track" ) );
    d->trackIcon = addRightHeaderAction( trackAction );
    connect( d->trackIcon, SIGNAL(clicked()), this, SLOT(_gotoTrack()) );

    QAction* settingsAction = new QAction( this );
    settingsAction->setIcon( QIcon::fromTheme( "preferences-system" ) );
    settingsAction->setText( i18n( "Settings" ) );
    d->settingsIcon = addRightHeaderAction( settingsAction );
    connect( d->settingsIcon, SIGNAL(clicked()), this, SLOT(showConfigurationInterface()) );

    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout( Qt::Vertical );
    layout->setSpacing( 2 );
    layout->addItem( m_header );
    layout->addItem( d->webView );
    setLayout( layout );

    dataEngine( "amarok-wikipedia" )->connectSource( "wikipedia", this );
    d->dataContainer = dataEngine( "amarok-wikipedia" )->containerForSource( "wikipedia" );

    // Read config and inform the engine.
    d->langList = Amarok::config("Wikipedia Applet").readEntry( "PreferredLang", QStringList() << "en" );
    d->useMobileWikipedia = Amarok::config("Wikipedia Applet").readEntry( "UseMobile", false );
    d->useSSL = Amarok::config( "Wikipedia Applet" ).readEntry( "UseSSL", true );
    d->_paletteChanged( App::instance()->palette() );
    d->dataContainer->setData( "lang", d->langList );
    d->dataContainer->setData( "mobile", d->useMobileWikipedia );
    d->dataContainer->setData( "ssl", d->useSSL );
    d->scheduleEngineUpdate();

    updateConstraints();
}

void
WikipediaApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Context::Applet::constraintsEvent( constraints );
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
WikipediaApplet::dataUpdated( const QString &source, const Plasma::DataEngine::Data &data )
{
    DEBUG_BLOCK
    Q_UNUSED( source )
    Q_D( WikipediaApplet );

    if( data.isEmpty() )
    {
        debug() << "data Empty!";
        d->webView->hide();
        setCollapseOn();
        return;
    }

    if( data.contains( "stopped" ) )
    {
        debug() << "stopped";
        d->dataContainer->removeAllData();
        if( d->webView->title().isEmpty() )
        {
            d->webView->hide();
            setCollapseOn();
        }
        return;
    }

    if( data.contains( "busy" ) )
    {
        if( canAnimate() && data["busy"].toBool() )
            setBusy( true );
        return;
    }
    else
    {
        d->webView->show();
        setBusy( false );
    }

    if( data.contains( "message" ) )
    {
        setCollapseOn();
        // messages have higher priority than pages
        const QString &message = data.value( "message" ).toString();
        if( !message.isEmpty() )
        {
            setHeaderText( i18n( "Wikipedia: %1", message ) );
            d->dataContainer->removeAllData();
        }
    }
    else if( data.contains( "sourceUrl" ) )
    {
        const QUrl &url = data.value( "sourceUrl" ).value<QUrl>();
        d->setUrl( url );
        debug() << "source URL" << url;
        setCollapseOff();

    }
    else if( data.contains( "page" ) )
    {
        if( data.contains( "url" ) && !data.value( "url" ).toUrl().isEmpty() )
        {
            const QUrl &url = data.value( "url" ).toUrl();
            d->_updateWebFonts();
            d->currentUrl = url;
            d->webView->setHtml( data[ "page" ].toString(), url );
            d->dataContainer->removeAllData();
        }
        setCollapseOff();
    }
    else
    {
        setHeaderText( i18n( "Wikipedia" ) );
    }
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

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    parent->setLayout(mainLayout);
    mainLayout->addWidget(mainWidget);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    parent->connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    parent->connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    mainLayout->addWidget(buttonBox);

    KConfigGroup configuration = config();
    QWidget *langSettings = new QWidget;
    d->languageSettingsUi.setupUi( langSettings );
    d->languageSettingsUi.downloadButton->setGuiItem( KStandardGuiItem::find() );
    d->languageSettingsUi.langSelector->availableListWidget()->setAlternatingRowColors( true );
    d->languageSettingsUi.langSelector->selectedListWidget()->setAlternatingRowColors( true );
    d->languageSettingsUi.langSelector->availableListWidget()->setUniformItemSizes( true );
    d->languageSettingsUi.langSelector->selectedListWidget()->setUniformItemSizes( true );
    d->languageSettingsUi.progressBar->setEnabled( false );

    QWidget *genSettings = new QWidget;
    d->generalSettingsUi.setupUi( genSettings );
    d->generalSettingsUi.mobileCheckBox->setCheckState( d->useMobileWikipedia ? Qt::Checked : Qt::Unchecked );
    d->generalSettingsUi.sslCheckBox->setCheckState( d->useSSL ? Qt::Checked : Qt::Unchecked );

    connect( d->languageSettingsUi.downloadButton, SIGNAL(clicked()), this, SLOT(_getLangMap()) );
    connect( parent, SIGNAL(clicked()), this, SLOT(_loadSettings()) );

    parent->addPage( genSettings, i18n( "Wikipedia General Settings" ), "configure" );
    parent->addPage( langSettings, i18n( "Wikipedia Language Settings" ), "applications-education-language" );
    QTimer::singleShot( 0, this, SLOT(_configureLangSelector()) );
}


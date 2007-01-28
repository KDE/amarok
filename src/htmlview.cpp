// (c) 2005 Christian Muehlhaeuser <chris@chris.de>
// (c) 2006 Seb Ruiz <me@sebruiz.net>
// License: GNU General Public License V2


#include "amarok.h"
#include "amarokconfig.h"
#include "app.h"
#include "contextbrowser.h"
#include "htmlview.h"
#include "playlist.h"      //appendMedia()

#include <qclipboard.h>
#include <qfile.h> // External CSS opening
#include <qimage.h> // External CSS opening

#include <kapplication.h> //kapp
#include <kactioncollection.h>
#include <kglobal.h> //kapp
#include <kimageeffect.h> // gradient background image
#include <kpopupmenu.h>
#include <kstandarddirs.h> //locate file
#include <ktempfile.h>

KTempFile *HTMLView::m_bgGradientImage = 0;
KTempFile *HTMLView::m_headerGradientImage = 0;
KTempFile *HTMLView::m_shadowGradientImage = 0;
int HTMLView::m_instances = 0;

HTMLView::HTMLView( QWidget *parentWidget, const char *widgetname, const bool DNDEnabled, const bool JScriptEnabled )
        : KHTMLPart( parentWidget, widgetname )
{
    m_instances++;
    setJavaEnabled( false );
    setPluginsEnabled( false );

    setDNDEnabled( DNDEnabled );
    setJScriptEnabled( JScriptEnabled );

    KActionCollection* ac = actionCollection();
    ac->setAutoConnectShortcuts( true );
    m_copy = KStdAction::copy( this, SLOT( copyText() ), ac, "htmlview_copy" );
    m_selectAll = KStdAction::selectAll( this, SLOT( selectAll() ), ac, "htmlview_select_all" );
    {
        KPopupMenu m;
        m_copy->plug( &m );
        m_selectAll->plug( &m );

        m_copy->unplug( &m );
        m_selectAll->unplug( &m );
    }

    connect( this, SIGNAL( selectionChanged() ), SLOT( enableCopyAction() ) );
    enableCopyAction();
}


HTMLView::~HTMLView()
{
    m_instances--;
    if ( m_instances < 1 ) {
        delete m_bgGradientImage;
        delete m_headerGradientImage;
        delete m_shadowGradientImage;
    }
}

void
HTMLView::enableCopyAction()
{
    m_copy->setEnabled( hasSelection() );
}

void
HTMLView::selectAll()
{
    KHTMLPart::selectAll();
}

void
HTMLView::copyText()
{
    QString text = selectedText();

    // Copy both to clipboard and X11-selection
    QApplication::clipboard()->setText( text, QClipboard::Clipboard );
    QApplication::clipboard()->setText( text, QClipboard::Selection );
}

void HTMLView::paletteChange() {
    delete m_bgGradientImage;
    delete m_headerGradientImage;
    delete m_shadowGradientImage;
    m_bgGradientImage = m_headerGradientImage = m_shadowGradientImage = 0;
}

QString
HTMLView::loadStyleSheet()
{
    QString themeName = AmarokConfig::contextBrowserStyleSheet().latin1();
    const QString file = kapp->dirs()->findResource( "data","amarok/themes/" + themeName + "/stylesheet.css" );

    QString styleSheet;
    if ( themeName != "Default" && QFile::exists( file ) )
    {
        const QString CSSLocation = kapp->dirs()->findResource( "data","amarok/themes/" + themeName + "/stylesheet.css" );
        QFile ExternalCSS( CSSLocation );
        if ( !ExternalCSS.open( IO_ReadOnly ) )
            return QString(); //FIXME: should actually return the default style sheet, then

        const QString pxSize = QString::number( ContextBrowser::instance()->fontMetrics().height() - 4 );
        const QString fontFamily = AmarokConfig::useCustomFonts() ?
                                        AmarokConfig::contextBrowserFont().family() :
                                        QApplication::font().family();
        const QString text   = ContextBrowser::instance()->colorGroup().text().name();
        const QString link   = ContextBrowser::instance()->colorGroup().link().name();
        const QString fg     = ContextBrowser::instance()->colorGroup().highlightedText().name();
        const QString bg     = ContextBrowser::instance()->colorGroup().highlight().name();
        const QString base   = ContextBrowser::instance()->colorGroup().base().name();
        const QColor bgColor = ContextBrowser::instance()->colorGroup().highlight();
        QColor gradientColor = bgColor;

        //we have to set the color for body due to a KHTML bug
        //KHTML sets the base color but not the text color
        styleSheet = QString( "body { margin: 8px; font-size: %1px; color: %2; background-color: %3; font-family: %4; }" )
                .arg( pxSize )
                .arg( text )
                .arg( AmarokConfig::schemeAmarok() ? fg : gradientColor.name() )
                .arg( fontFamily );

        QTextStream eCSSts( &ExternalCSS );
        QString tmpCSS = eCSSts.read();
        ExternalCSS.close();

        tmpCSS.replace( "./", KURL::fromPathOrURL( CSSLocation ).directory( false ) );
        tmpCSS.replace( "AMAROK_FONTSIZE-2", pxSize );
        tmpCSS.replace( "AMAROK_FONTSIZE", pxSize );
        tmpCSS.replace( "AMAROK_FONTSIZE+2", pxSize );
        tmpCSS.replace( "AMAROK_FONTFAMILY", fontFamily );
        tmpCSS.replace( "AMAROK_TEXTCOLOR", text );
        tmpCSS.replace( "AMAROK_LINKCOLOR", link );
        tmpCSS.replace( "AMAROK_BGCOLOR", bg );
        tmpCSS.replace( "AMAROK_FGCOLOR", fg );
        tmpCSS.replace( "AMAROK_BASECOLOR", base );
        tmpCSS.replace( "AMAROK_DARKBASECOLOR", ContextBrowser::instance()->colorGroup().base().dark( 120 ).name() );
        tmpCSS.replace( "AMAROK_GRADIENTCOLOR", gradientColor.name() );

        styleSheet += tmpCSS;
    }
    else
    {
        int pxSize = ContextBrowser::instance()->fontMetrics().height() - 4;
        const QString fontFamily = AmarokConfig::useCustomFonts() ? AmarokConfig::contextBrowserFont().family() : QApplication::font().family();
        const QString text = ContextBrowser::instance()->colorGroup().text().name();
        const QString link = ContextBrowser::instance()->colorGroup().link().name();
        const QString fg   = ContextBrowser::instance()->colorGroup().highlightedText().name();
        const QString bg   = ContextBrowser::instance()->colorGroup().highlight().name();
        const QColor baseColor = ContextBrowser::instance()->colorGroup().base();
        const QColor bgColor = ContextBrowser::instance()->colorGroup().highlight();
        const QColor gradientColor = bgColor;

        if ( !m_bgGradientImage ) {
            m_bgGradientImage = new KTempFile( locateLocal( "tmp", "gradient" ), ".png", 0600 );
            QImage image = KImageEffect::gradient( QSize( 600, 1 ), gradientColor, gradientColor.light( 130 ), KImageEffect::PipeCrossGradient );
            image.save( m_bgGradientImage->file(), "PNG" );
            m_bgGradientImage->close();
        }

        if ( !m_headerGradientImage ) {
            m_headerGradientImage = new KTempFile( locateLocal( "tmp", "gradient_header" ), ".png", 0600 );
            QImage imageH = KImageEffect::unbalancedGradient( QSize( 1, 10 ), bgColor, gradientColor.light( 130 ), KImageEffect::VerticalGradient, 100, -100 );
            imageH.copy( 0, 1, 1, 9 ).save( m_headerGradientImage->file(), "PNG" );
            m_headerGradientImage->close();
        }

        if ( !m_shadowGradientImage ) {
            m_shadowGradientImage = new KTempFile( locateLocal( "tmp", "gradient_shadow" ), ".png", 0600 );
            QImage imageS = KImageEffect::unbalancedGradient( QSize( 1, 10 ), baseColor, Qt::gray, KImageEffect::VerticalGradient, 100, -100 );
            imageS.save( m_shadowGradientImage->file(), "PNG" );
            m_shadowGradientImage->close();
        }

        //unlink the files for us on deletion
        m_bgGradientImage->setAutoDelete( true );
        m_headerGradientImage->setAutoDelete( true );
        m_shadowGradientImage->setAutoDelete( true );

        //we have to set the color for body due to a KHTML bug
        //KHTML sets the base color but not the text color
        styleSheet = QString( "body { margin: 4px; font-size: %1px; color: %2; background-color: %3; background-image: url( %4 ); background-repeat: repeat; font-family: %5; }" )
                .arg( pxSize )
                .arg( text )
                .arg( AmarokConfig::schemeAmarok() ? fg : gradientColor.name() )
                .arg( m_bgGradientImage->name() )
                .arg( fontFamily );

        //text attributes
        styleSheet += QString( "h1 { font-size: %1px; }" ).arg( pxSize + 8 );
        styleSheet += QString( "h2 { font-size: %1px; }" ).arg( pxSize + 6 );
        styleSheet += QString( "h3 { font-size: %1px; }" ).arg( pxSize + 4 );
        styleSheet += QString( "h4 { font-size: %1px; }" ).arg( pxSize + 3 );
        styleSheet += QString( "h5 { font-size: %1px; }" ).arg( pxSize + 2 );
        styleSheet += QString( "h6 { font-size: %1px; }" ).arg( pxSize + 1 );
        styleSheet += QString( "a { font-size: %1px; color: %2; }" ).arg( pxSize ).arg( text );
        styleSheet += QString( ".info { display: block; margin-left: 4px; font-weight: normal; }" );

        styleSheet += QString( ".song a { display: block; padding: 1px 2px; font-weight: normal; text-decoration: none; }" );
        styleSheet += QString( ".song a:hover { color: %1; background-color: %2; }" ).arg( fg ).arg( bg );
        styleSheet += QString( ".song-title { font-weight: bold; }" );
        styleSheet += QString( ".song-place { font-size: %1px; font-weight: bold; }" ).arg( pxSize + 3 );

        //box: the base container for every block (border hilighted on hover, 'A' without underlining)
        styleSheet += QString( ".box { border: solid %1 1px; text-align: left; margin-bottom: 10px; }" ).arg( bg );
        styleSheet += QString( ".box a { text-decoration: none; }" );
        styleSheet += QString( ".box:hover { border: solid %1 1px; }" ).arg( text );

        //box contents: header, body, rows and alternate-rows
        styleSheet += QString( ".box-header { color: %1; background-color: %2; background-image: url( %4 ); background-repeat: repeat-x; font-size: %3px; font-weight: bold; padding: 1px 0.5em; border-bottom: 1px solid #000; }" )
                .arg( fg )
                .arg( bg )
                .arg( pxSize + 2 )
                .arg( m_headerGradientImage->name() );

        styleSheet += QString( ".box-body { padding: 2px; background-color: %1; background-image: url( %2 ); background-repeat: repeat-x; font-size:%3px; }" )
                .arg( ContextBrowser::instance()->colorGroup().base().name() )
                .arg( m_shadowGradientImage->name() )
                .arg( pxSize );

        //"Related Artists" related styles
        styleSheet += QString( ".box-header-nav { color: %1; background-color: %2; font-size: %3px; font-weight: bold; padding: 1px 0.5em; border-bottom: 1px solid #000; text-align: right; }" )
                .arg( fg )
                .arg( bg )
                .arg( pxSize );

        //"Albums by ..." related styles
        styleSheet += QString( ".album-header:hover { color: %1; background-color: %2; cursor: pointer; }" ).arg( fg ).arg( bg );
        styleSheet += QString( ".album-header:hover a { color: %1; }" ).arg( fg );
        styleSheet += QString( ".album-body { background-color: %1; border-bottom: solid %2 1px; border-top: solid %3 1px; }" ).arg( ContextBrowser::instance()->colorGroup().base().name() ).arg( bg ).arg( bg );
        styleSheet += QString( ".album-title { font-weight: bold; }" );
        styleSheet += QString( ".album-info { float:right; padding-right:4px; font-size: %1px }" ).arg( pxSize );
        styleSheet += QString( ".album-length { float:right; padding-right:4px; font-size: %1px; clear:right; }" ).arg( pxSize );
        styleSheet += QString( ".album-image { padding-right: 4px; }" );
        styleSheet += QString( ".album-song a { display: block; padding: 1px 2px; font-weight: normal; text-decoration: none; }" );
        styleSheet += QString( ".album-song a:hover { color: %1; background-color: %2; }" ).arg( fg ).arg( bg );
        styleSheet += QString( ".album-song-trackno { font-weight: bold; }" );

        styleSheet += QString( ".disc-separator { color: %1; border-bottom: 1px solid %2; }" ).arg( bg ).arg( bg );
        styleSheet += QString( ".disc-separator a { display: block; padding: 1px 2px; font-weight: normal; text-decoration: none; }" );
        styleSheet += QString( ".disc-separator a:hover { color: %1; background-color: %2; }" ).arg( fg ).arg( bg );

        styleSheet += QString( ".button { width: 100%; }" );

        //boxes used to display score (sb: score box)
        styleSheet += QString( ".sbtext { text-align: right; padding: 0px 4px; }" );
        styleSheet += QString( ".sbinner { height: 8px; background-color: %1; border: solid %2 1px; }" )
                            .arg( ContextBrowser::instance()->colorGroup().highlight().name() )
                            .arg( ContextBrowser::instance()->colorGroup().highlightedText().name() );
        styleSheet += QString( ".sbouter { width: 52px; height: 10px; background-color: %1; border: solid %2 1px; }" )
                            .arg( ContextBrowser::instance()->colorGroup().base().dark( 120 ).name() )
                            .arg( ContextBrowser::instance()->colorGroup().highlight().name() );

        styleSheet += QString( ".ratingBox { padding: 0px 4px; }" );
        styleSheet += QString( ".ratingStar { height: 0.9em; }" );

        styleSheet += QString( ".statsBox { border-left: solid %1 1px; }" )
                      .arg( ContextBrowser::instance()->colorGroup().base().dark( 120 ).name() );

        styleSheet += QString( "#current_box-header-album { font-weight: normal; }" );
        styleSheet += QString( "#current_box-information-td { text-align: right; vertical-align: bottom; padding: 3px; }" );
        styleSheet += QString( "#current_box-largecover-td { text-align: left; width: 100px; padding: 0; vertical-align: bottom; }" );
        styleSheet += QString( "#current_box-largecover-image { padding: 4px; vertical-align: bottom; }" );

        styleSheet += QString( "#wiki_box-body a { color: %1; }" ).arg( link );
        styleSheet += QString( "#wiki_box-body a:hover { text-decoration: underline; }" );

        //labels in tag dialog
        styleSheet += ".label a:hover { font-weight: bold; }";
        styleSheet += QString( ".label.size1 { font-size: %1px; }" ).arg( pxSize );
        styleSheet += QString( ".label.size2 { font-size: %1px; }" ).arg( pxSize + 1 );
        styleSheet += QString( ".label.size3 { font-size: %1px; }" ).arg( pxSize + 2 );
        styleSheet += QString( ".label.size4 { font-size: %1px; }" ).arg( pxSize + 3 );
        styleSheet += QString( ".label.size5 { font-size: %1px; }" ).arg( pxSize + 4);
        styleSheet += QString( ".label.size6 { font-size: %1px; }" ).arg( pxSize + 5 );
        styleSheet += QString( ".label.size7 { font-size: %1px; }" ).arg( pxSize + 6 );
        styleSheet += QString( ".label.size8 { font-size: %1px; }" ).arg( pxSize + 7 );
        styleSheet += QString( ".label.size9 { font-size: %1px; }" ).arg( pxSize + 8 );
        styleSheet += QString( ".label.size10 { font-size: %1px; }" ).arg( pxSize + 9 );
    }

    return styleSheet;
}


void
HTMLView::set( const QString& data )
{
    begin();
    setUserStyleSheet( loadStyleSheet() );
    write( data );
    end();
}


void HTMLView::openURLRequest( const KURL &url )
{
    // here, http urls are streams. For webpages we use externalurl
    // NOTE there have been no links to streams! http now used for wiki tab.
    if ( url.protocol() == "file" )
        Playlist::instance()->insertMedia( url, Playlist::DefaultOptions );
}

#include "htmlview.moc"

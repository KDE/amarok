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

#include "WikipediaApplet.h"

#include "Amarok.h"
#include "App.h"
#include "Debug.h"
#include "context/Svg.h"
#include "EngineController.h"
#include "PaletteHandler.h"

#include <plasma/theme.h>
#include <plasma/widgets/webview.h>
#include <plasma/widgets/iconwidget.h>

#include <KIcon>
#include <KStandardDirs>

#include <QAction>
#include <QDesktopServices>
#include <QGraphicsSimpleTextItem>
#include <QPainter>

WikipediaApplet::WikipediaApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_aspectRatio( 0 )
    , m_headerAspectRatio( 0.0 )
    , m_size( QSizeF() )
    , m_wikipediaLabel( 0 )
    , m_webView( 0 )
    , m_reloadIcon( 0 )
    , m_css( 0 )
{
    setHasConfigurationInterface( false );
    setBackgroundHints( Plasma::Applet::NoBackground );
}

WikipediaApplet::~ WikipediaApplet()
{
    delete m_webView;
    delete m_css;
}

void WikipediaApplet::init()
{
    m_wikipediaLabel = new QGraphicsSimpleTextItem( this );

    m_webView = new Plasma::WebView( this );
    m_webView->setAttribute( Qt::WA_NoSystemBackground );

    paletteChanged( App::instance()->palette() );
    connect( The::paletteHandler(), SIGNAL( newPalette( const QPalette& ) ), SLOT(  paletteChanged( const QPalette &  ) ) );

    m_webView->page()->setLinkDelegationPolicy ( QWebPage::DelegateAllLinks );
    connect( m_webView->page(), SIGNAL( linkClicked( const QUrl & ) ) , this, SLOT( linkClicked ( const QUrl & ) ) );

    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_wikipediaLabel->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    m_wikipediaLabel->setFont( labelFont );
    m_wikipediaLabel->setText( i18n( "Wikipedia" ) );

    QAction* reloadAction = new QAction( i18n( "Reload" ), this );
    reloadAction->setIcon( KIcon( "view-refresh" ) );
    reloadAction->setVisible( true );
    reloadAction->setEnabled( false );
    m_reloadIcon = addAction( reloadAction );

    connect( m_reloadIcon, SIGNAL( activated() ), this, SLOT( reloadWikipedia() ) );

    connectSource( "wikipedia" );
    connect( dataEngine( "amarok-wikipedia" ), SIGNAL( sourceAdded( const QString & ) ),
             this, SLOT( connectSource( const QString & ) ) );

    constraintsEvent();
}

Plasma::IconWidget *
WikipediaApplet::addAction( QAction *action )
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
WikipediaApplet::connectSource( const QString &source )
{
    if( source == "wikipedia" )
        dataEngine( "amarok-wikipedia" )->connectSource( "wikipedia", this );
}

void
WikipediaApplet::linkClicked( const QUrl &url )
{
    debug() << "URL: " << url;
    QDesktopServices::openUrl( url.toString() );
}

void WikipediaApplet::constraintsEvent( Plasma::Constraints constraints )
{

    prepareGeometryChange();

    float textWidth = m_wikipediaLabel->boundingRect().width();
    float offsetX =  ( boundingRect().width() - textWidth ) / 2;

    m_wikipediaLabel->setPos( offsetX, 6 );

    m_webView->setPos( 6, 30 );
    m_webView->resize( boundingRect().width() - 12, 415 );

    m_reloadIcon->setPos( size().width() - m_reloadIcon->size().width() - MARGIN, MARGIN );
}

bool WikipediaApplet::hasHeightForWidth() const
{
    return true;
}

qreal WikipediaApplet::heightForWidth( qreal width ) const
{
    return width * m_aspectRatio;
}

void WikipediaApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data ) // SLOT
{
    Q_UNUSED( name )
    debug() << "WikipediaApplet::dataUpdated: " << name;

    if( data.size() == 0 ) return;

    if( data.contains( "page" ) )
        m_webView->setHtml( data[ "page" ].toString(), KUrl( QString() ) );
    else
        m_webView->setHtml( data[ data.keys()[ 0 ] ].toString(), KUrl( QString() ) ); // set data

    if( data.contains( "label" ) )
        m_label = data[ "label" ].toString() + ':';
    else
        m_label.clear();

    if( data.contains( "title" ) )
        m_title = data[ "title" ].toString();
    else
        m_title.clear();

    if( m_reloadIcon->action() && !m_reloadIcon->action()->isEnabled() )
    {        
        m_reloadIcon->action()->setEnabled( true );
        //for some reason when we enable the action suddenly the icon has the text "..."
        m_reloadIcon->action()->setText( "" );  
    }
}

void WikipediaApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option )
    Q_UNUSED( contentsRect )
    p->setRenderHint( QPainter::Antialiasing );

    p->save();
    QLinearGradient gradient( boundingRect().topLeft(), boundingRect().bottomLeft() );
    QColor highlight = Amarok::highlightColor();
    highlight.setAlpha( 80 );
    gradient.setColorAt( 0, highlight );
    highlight.setAlpha( 200 );
    gradient.setColorAt( 1, highlight );
    QPainterPath path;
    path.addRoundedRect( boundingRect(), 3, 3 );
    p->fillPath( path, gradient );
    p->restore();

        // draw rounded rect around title
    p->save();
    QColor topColor( 255, 255, 255, 120 );
    QLinearGradient gradient2( m_wikipediaLabel->boundingRect().topLeft(), m_wikipediaLabel->boundingRect().bottomLeft() );
    topColor.setAlpha( 120 );
    gradient2.setColorAt( 0, topColor );
    topColor.setAlpha( 200 );
    gradient2.setColorAt( 1, topColor );
    path = QPainterPath();
    QRectF titleRect = m_wikipediaLabel->boundingRect();
    titleRect.moveTopLeft( m_wikipediaLabel->pos() );
    path.addRoundedRect( titleRect.adjusted( -3, 0, 3, 0 ), 5, 5 );
    p->fillPath( path, gradient2 );
    p->restore();
    
}

QSizeF WikipediaApplet::sizeHint( Qt::SizeHint which, const QSizeF & constraint ) const
{
    // TODO hardcoding for now.
    // i want to have a system where an applet can ask
    // for a full "CV pane" of size, but for now this will stop the crash
    QSizeF size;
    size.setWidth( QGraphicsWidget::sizeHint( which, constraint ).width() );
    size.setHeight( 450 );
    return size;
}

void
WikipediaApplet::reloadWikipedia()
{
    DEBUG_BLOCK
    dataEngine( "amarok-wikipedia" )->query( "wikipedia:reload" );
}

void
WikipediaApplet::paletteChanged( const QPalette & palette )
{

  //  m_webView->setStyleSheet( QString( "QTextBrowser { background-color: %1; border-width: 0px; border-radius: 0px; color: %2; }" ).arg( Amarok::highlightColor().lighter( 150 ).name() )
  //                                                                                                            .arg( Amarok::highlightColor().darker( 400 ).name() ) );
    //m_webView->page()->settings()->setUserStyleSheetUrl( "file://" + KStandardDirs::locate("data", "amarok/data/WikipediaCustomStyle.css" ) );
    // read css, replace color placeholders, write to file, load into page
    QFile file( KStandardDirs::locate("data", "amarok/data/WikipediaCustomStyle.css" ) );
    if( file.open(QIODevice::ReadOnly | QIODevice::Text) )
    {
        QColor highlight( App::instance()->palette().highlight().color() );
        qreal saturation = highlight.saturationF();
        saturation *= 0.3;
        qreal value = highlight.valueF();
        value *= 1.1;
        debug() << "value:" << highlight.valueF();
        highlight.setHsvF( highlight.hueF(), 0.05, 1, highlight.alphaF() );
        
        QString contents = QString( file.readAll() );
        //debug() << "setting background:" << Amarok::highlightColor().lighter( 130 ).name();
        contents.replace( "{background_color}", highlight.name() );
        contents.replace( "{text_color}", palette.brush( QPalette::Text ).color().name() );
        contents.replace( "{link_color}", palette.link().color().name() );
        contents.replace( "{link_hover_color}", palette.link().color().darker( 200 ).name() );
        highlight.setHsvF( highlight.hueF(), 0.3, .95, highlight.alphaF() );
        contents.replace( "{shaded_text_background_color}", highlight.name() );

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


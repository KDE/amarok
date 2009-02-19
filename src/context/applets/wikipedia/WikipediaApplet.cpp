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
#include "Debug.h"
#include "context/Svg.h"
#include "EngineController.h"

#include <plasma/theme.h>
#include <plasma/widgets/webview.h>
#include <plasma/widgets/iconwidget.h>

#include <KIcon>
#include <KStandardDirs>

#include <QAction>
#include <QGraphicsSimpleTextItem>
#include <QPainter>

WikipediaApplet::WikipediaApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_theme( 0 )
    , m_header( 0 )
    , m_aspectRatio( 0 )
    , m_headerAspectRatio( 0.0 )
    , m_size( QSizeF() )
    , m_wikipediaLabel( 0 )
    , m_webView( 0 )
    , m_reloadIcon( 0 )
{
    setHasConfigurationInterface( false );
    setBackgroundHints( Plasma::Applet::NoBackground );
}

WikipediaApplet::~ WikipediaApplet()
{
    //hacky stuff to keep QWebView from causing a crash
   /* m_wikiPage->setWidget( 0 );
    delete m_wikiPage;
    m_wikiPage = 0;
    delete m_webView;*/

    delete m_webView;
}

void WikipediaApplet::init()
{
    m_theme = new Plasma::FrameSvg( this );
    QString imagePath = KStandardDirs::locate("data", "amarok/images/web_applet_background.svg" );

    debug() << "Loading theme file: " << imagePath;
    
    m_theme->setImagePath( imagePath );
    m_theme->setContainsMultipleImages( true );
    m_theme->setEnabledBorders( Plasma::FrameSvg::AllBorders );

    m_header = new Context::Svg( this );
    m_header->setImagePath( "widgets/amarok-wikipedia" );
    m_header->setContainsMultipleImages( false );
    
    m_header->resize();
    m_aspectRatio = (qreal)m_header->size().height() / (qreal)m_header->size().width();
    m_size = m_header->size();

    m_wikipediaLabel = new QGraphicsSimpleTextItem( this );

    m_webView = new Plasma::WebView( this );

    m_webView->page()->settings()->setUserStyleSheetUrl( "file://" + KStandardDirs::locate("data", "amarok/data/WikipediaCustomStyle.css" ) );
    m_webView->page()->setLinkDelegationPolicy ( QWebPage::DelegateAllLinks );
    connect( m_webView->page(), SIGNAL( linkClicked( const QUrl & ) ) , this, SLOT( linkClicked ( const QUrl & ) ) );

    //make background transparent
    QPalette p = m_webView->palette();
    p.setColor( QPalette::Dark, QColor( 255, 255, 255, 0)  );
    p.setColor( QPalette::Window, QColor( 255, 255, 255, 0)  );
    m_webView->setPalette( p );

    QFont labelFont;
    labelFont.setBold( true );
    labelFont.setPointSize( labelFont.pointSize() + 3 );
    m_wikipediaLabel->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    m_wikipediaLabel->setFont( labelFont );
    m_wikipediaLabel->setText( i18n( "Wikipedia" ) );

    connectSource( "wikipedia" );
    connect( dataEngine( "amarok-wikipedia" ), SIGNAL( sourceAdded( const QString & ) ),
             this, SLOT( connectSource( const QString & ) ) );

    QAction* reloadAction = new QAction( i18n( "Reload" ), this );
    reloadAction->setIcon( KIcon( "view-refresh" ) );
    reloadAction->setVisible( true );
    reloadAction->setEnabled( false );
    m_reloadIcon = addAction( reloadAction );

    connect( m_reloadIcon, SIGNAL( activated() ), this, SLOT( reloadWikipedia() ) );
    
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

    Amarok::invokeBrowser( url.toString() );
}

void WikipediaApplet::constraintsEvent( Plasma::Constraints constraints )
{
    if( !m_header )
        return;

    prepareGeometryChange();
    if ( constraints & Plasma::SizeConstraint )
        m_header->resize( size().toSize() );

    m_theme->resizeFrame(size().toSize());

    m_wikipediaLabel->setFont( shrinkTextSizeToFit( "Wikipedia", m_header->elementRect( "wikipedialabel" ) ) );

    float textWidth = m_wikipediaLabel->boundingRect().width();
    float totalWidth = m_header->elementRect( "wikipedialabel" ).width();
    float offsetX =  ( totalWidth - textWidth ) / 2;

    m_wikipediaLabel->setPos( offsetX, m_header->elementRect( "wikipedialabel" ).topLeft().y() );

    m_webView->setPos( m_header->elementRect( "wikipediainformation" ).topLeft() );

    QSizeF infoSize( m_header->elementRect( "wikipediainformation" ).bottomRight().x() - m_header->elementRect( "wikipediainformation" ).topLeft().x(), m_header->elementRect( "wikipediainformation" ).bottomRight().y() - m_header->elementRect( "wikipediainformation" ).topLeft().y() );

    if ( infoSize.isValid() )
        m_webView->resize( infoSize );

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

    if( !m_reloadIcon->action()->isEnabled() )
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

    m_header->resize(size().toSize());
    m_theme->resizeFrame(size().toSize());
    p->save();

    m_theme->paintFrame( p, QRectF( 0.0, 0.0, size().toSize().width(), size().toSize().height() )/*, "header" */);

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

#include "WikipediaApplet.moc"


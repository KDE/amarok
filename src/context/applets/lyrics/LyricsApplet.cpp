/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "LyricsApplet.h"

#include "Amarok.h"
#include "App.h"
#include "Debug.h"
#include "EngineController.h"
#include "dialogs/ScriptManager.h"
#include "meta/Meta.h"
#include "PaletteHandler.h"
#include "Theme.h"

#include <KGlobalSettings>
#include <KStandardDirs>

#include <QAction>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsProxyWidget>
#include <QLinearGradient>
#include <QTextBrowser>
#include <QPainter>
#include <QPoint>

LyricsApplet::LyricsApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_titleText( i18n( "Lyrics" ) )
    , m_titleLabel( 0 )
    , m_reloadIcon( 0 )
    , m_lyrics( 0 )
    , m_suggested( 0 )
{
    setHasConfigurationInterface( false );
    setBackgroundHints( Plasma::Applet::NoBackground );

}

LyricsApplet::~ LyricsApplet()
{
    m_lyricsProxy->setWidget( 0 );
    delete m_lyricsProxy;
    m_lyricsProxy = 0;
    delete m_lyrics;
}

void LyricsApplet::init()
{
    m_titleLabel = new QGraphicsSimpleTextItem( i18n( "Lyrics" ), this );
    QFont bigger = m_titleLabel->font();
    bigger.setPointSize( bigger.pointSize() + 2 );
    m_titleLabel->setFont( bigger );
    m_titleLabel->setZValue( m_titleLabel->zValue() + 100 );
    
    QAction* reloadAction = new QAction( i18n( "Reload Lyrics" ), this );
    reloadAction->setIcon( KIcon( "view-refresh" ) );
    reloadAction->setVisible( true );
    reloadAction->setEnabled( true );
    m_reloadIcon = addAction( reloadAction );

    connect( m_reloadIcon, SIGNAL( activated() ), this, SLOT( refreshLyrics() ) );
    
    m_lyricsProxy = new QGraphicsProxyWidget( this );
    m_lyrics = new QTextBrowser;
    m_lyrics->setAttribute( Qt::WA_NoSystemBackground );
    m_lyrics->setReadOnly( true );
    m_lyrics->setOpenExternalLinks( true );
    m_lyrics->setWordWrapMode( QTextOption::WordWrap );
    m_lyrics->setTextInteractionFlags( Qt::TextBrowserInteraction | Qt::TextSelectableByKeyboard );
    m_lyricsProxy->setWidget( m_lyrics );
    QPalette pal;
    QBrush brush(  PaletteHandler::highlightColor().lighter( 170 ) );
    brush.setStyle( Qt::SolidPattern );
    pal.setBrush( QPalette::Active, QPalette::Base, brush );
    pal.setBrush( QPalette::Inactive, QPalette::Base, brush );
    pal.setBrush( QPalette::Disabled, QPalette::Base, brush );
    pal.setBrush( QPalette::Window, brush );
    m_lyrics->setPalette( pal );
    m_lyricsProxy->setPalette( pal );
    m_lyrics->setStyleSheet( QString( "QTextBrowser { background-color: %1; border-width: 0px; border-radius: 0px; color: %2; }" ).arg( PaletteHandler::highlightColor().lighter( 150 ).name() )
                                                                                                              .arg( PaletteHandler::highlightColor().darker( 400 ).name() ) );

    // only show when we need to let the user
    // choose between suggestions
    m_suggested = new QGraphicsTextItem( this );
    connect( m_suggested, SIGNAL( linkActivated( const QString& ) ), this, SLOT( suggestionChosen( const QString& ) ) );
    m_suggested->setTextInteractionFlags( Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard );
    m_suggested->hide();
    
    connect( dataEngine( "amarok-lyrics" ), SIGNAL( sourceAdded( const QString& ) ), this, SLOT( connectSource( const QString& ) ) );
    connect( The::paletteHandler(), SIGNAL( newPalette( const QPalette& ) ), SLOT(  paletteChanged( const QPalette &  ) ) );

    constraintsEvent();
    connectSource( "lyrics" );
}

Plasma::IconWidget*
LyricsApplet::addAction( QAction *action )
{
    if ( !action )
    {
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


    tool->hide();
    tool->setZValue( zValue() + 1 );

    return tool;
}

void LyricsApplet::connectSource( const QString& source )
{
    if( source == "lyrics" ) {
        dataEngine( "amarok-lyrics" )->connectSource( source, this );
        refreshLyrics(); // get data initally
    } else if( source == "suggested" )
    {
        dataEngine( "amarok-lyrics" )->connectSource( source, this );
        dataUpdated( source, dataEngine("amarok-lyrics" )->query( "suggested" ) ); 
    }
} 

void LyricsApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints );

    prepareGeometryChange();

    m_suggested->setTextWidth( size().width() );

    QRectF rect = boundingRect();
    rect.setWidth( rect.width() - 30 );
    m_titleLabel->setText( truncateTextToFit( m_titleText, m_titleLabel->font(), rect ) );
    m_titleLabel->setPos( (size().width() - m_titleLabel->boundingRect().width() ) / 2, standardPadding() + 2 );
    
    m_reloadIcon->setPos( size().width() - m_reloadIcon->size().width() - standardPadding(), standardPadding() );
    m_reloadIcon->show();
    
    //m_lyricsProxy->setPos( 0, m_reloadIcon->size().height() );
    m_lyricsProxy->setPos( standardPadding(), m_titleLabel->pos().y() + m_titleLabel->boundingRect().height() + standardPadding() );
    QSize lyricsSize( size().width() - 2 * standardPadding(), boundingRect().height() - m_lyricsProxy->pos().y() - standardPadding() );
    m_lyricsProxy->setMinimumSize( lyricsSize );
    m_lyricsProxy->setMaximumSize( lyricsSize );

    update();
}

void LyricsApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    Q_UNUSED( name )

    if( data.size() == 0 ) return;

    //debug() << "lyrics applet got name:" << name << "and lyrics: " << data;

    m_titleLabel->show();
    if( data.contains( "noscriptrunning" ) )
    {
        m_suggested->hide();
        m_lyrics->show();m_lyrics->setPlainText( i18n( "No lyrics script is running." ) );
    }
    else if( data.contains( "fetching" ) )
    {
        m_suggested->hide();
        m_lyrics->show();
        m_lyrics->setPlainText( i18n( "Lyrics are being fetched." ) );
    }
    else if( data.contains( "error" ) )
    {
        m_suggested->hide();
        m_lyrics->show();
        m_lyrics->setPlainText( i18n( "Lyrics were not able to be downloaded. Please check your internet connection: %1", data["error"].toString() ) );
    }
    else if( data.contains( "suggested" ) )
    {
        m_lyrics->hide();
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
        //debug() << "setting html: " << html;
        m_suggested->setHtml( html );
        m_suggested->show();
    }
    else if( data.contains( "html" ) )
    {
        // show pure html in the text area
        m_suggested->hide();
        m_lyrics->setHtml( data[ "html" ].toString() );
        m_lyrics->show();
    }
    else if( data.contains( "lyrics" ) )
    {
        m_suggested->hide();
        m_lyrics->show();
        QVariantList lyrics  = data[ "lyrics" ].toList();

        m_titleText = QString( " %1 : %2 - %3" ).arg( i18n( "Lyrics" ) ).arg( lyrics[ 0 ].toString() ).arg( lyrics[ 1 ].toString() );
        //  need padding for title
        m_lyrics->setPlainText( lyrics[ 3 ].toString().trimmed() );
    }
    else if( data.contains( "notfound" ) )
    {
        m_suggested->hide();
        m_lyrics->show();
        m_lyrics->setPlainText( i18n( "There were no lyrics found for this track" ) );
    }
    //setPreferredSize( (int)size().width(), (int)size().height() );
    updateConstraints();
    update();
}

bool LyricsApplet::hasHeightForWidth() const
{
    return false;
}

void
LyricsApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option );
    Q_UNUSED( contentsRect );
    p->setRenderHint( QPainter::Antialiasing );

    // tint the whole applet
    addGradientToAppletBackground( p );

    // draw rounded rect around title
    drawRoundedRectAroundText( p, m_titleLabel );

    //draw background of lyrics text
    p->save();
    QColor highlight( App::instance()->palette().highlight().color() );
    highlight.setHsvF( highlight.hueF(), 0.07, 1, highlight.alphaF() );

    // HACK
    // sometimes paint is done before the updateconstraints call
    // so m_lyricsProxy bounding rect is not yet correct
    QRectF lyricsRect(
        QPointF( standardPadding(), m_titleLabel->pos().y() + m_titleLabel->boundingRect().height() + standardPadding() ),
        QSizeF( size().width() - 2 * standardPadding(), boundingRect().height() - m_lyricsProxy->pos().y() - standardPadding() ) );

    lyricsRect.moveTopLeft( m_lyricsProxy->pos() );
    QPainterPath path;
    path.addRoundedRect( lyricsRect, 5, 5 );
    p->fillPath( path , highlight );
    p->restore();
    
}

QSizeF LyricsApplet::sizeHint(Qt::SizeHint which, const QSizeF & constraint) const
{
    Q_UNUSED( which );

    // ask for rest of CV height
    return QSizeF( QGraphicsWidget::sizeHint( which, constraint ).width(), -1 );
    
}


void
LyricsApplet::paletteChanged( const QPalette & palette )
{
    Q_UNUSED( palette )

    if( m_lyrics )
        m_lyrics->setStyleSheet( QString( "QTextBrowser { background-color: %1; border-width: 0px; border-radius: 0px; color: %2; }" ).arg( PaletteHandler::highlightColor().lighter( 150 ).name() )
                                                                                                              .arg( PaletteHandler::highlightColor().darker( 400 ).name() ) );
    
}

void
LyricsApplet::suggestionChosen( const QString& link )
{
    QStringList pieces = link.split( '|' );
    ScriptManager::instance()->notifyFetchLyricsByUrl( pieces[ 1 ], pieces[ 0 ], pieces[ 2 ] );
}

void
LyricsApplet::refreshLyrics()
{
    Meta::TrackPtr curtrack = The::engineController()->currentTrack();
    debug() << "checking for current track:";

    if( !curtrack || !curtrack->artist() )
        return;

    ScriptManager::instance()->notifyFetchLyrics( curtrack->artist()->name(), curtrack->name() );
}

#include "LyricsApplet.moc"

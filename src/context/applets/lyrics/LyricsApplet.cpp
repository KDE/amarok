/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 * Copyright (c) 2009 simon.esneault <simon.esneault@gmail.com>                         *
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
#include "context/widgets/TextScrollingWidget.h"
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
    , m_editIcon( 0 )
    , m_saveIcon( 0 )
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
    // properly set the size, asking for the whole cv size.
    resize( 500, -1 );
    
    m_titleLabel = new TextScrollingWidget( this );
    QFont bigger = m_titleLabel->font();
    bigger.setPointSize( bigger.pointSize() + 2 );
    m_titleLabel->setFont( bigger );
    m_titleLabel->setZValue( m_titleLabel->zValue() + 100 );
    m_titleLabel->setText( i18n( "Lyrics" ) );

    QAction* editAction = new QAction( i18n( "Edit Lyrics" ), this );
    editAction->setIcon( KIcon( "document-edit" ) );
    editAction->setVisible( true );
    editAction->setEnabled( false );
    m_editIcon = addAction( editAction );

    connect( m_editIcon, SIGNAL( activated() ), this, SLOT( editLyrics() ) );

    QAction* saveAction = new QAction( i18n( "Save Lyrics" ), this );
    saveAction->setIcon( KIcon( "document-save" ) );
    saveAction->setVisible( false );
    saveAction->setEnabled( false );
    m_saveIcon = addAction( saveAction );

    connect( m_saveIcon, SIGNAL( activated() ), this, SLOT( saveLyrics() ) );
    
    QAction* reloadAction = new QAction( i18n( "Reload Lyrics" ), this );
    reloadAction->setIcon( KIcon( "view-refresh" ) );
    reloadAction->setVisible( true );
    reloadAction->setEnabled( true );
    m_reloadIcon = addAction( reloadAction );

    connect( m_reloadIcon, SIGNAL( activated() ), this, SLOT( refreshLyrics() ) );

    m_lyricsProxy = new QGraphicsProxyWidget( this );
    m_lyrics = new QTextBrowser;
    m_lyrics->setAttribute( Qt::WA_NoSystemBackground );
    m_lyrics->setOpenExternalLinks( true );
    m_lyrics->setWordWrapMode( QTextOption::WordWrap );
    m_lyrics->setTextInteractionFlags( Qt::TextBrowserInteraction | Qt::TextSelectableByKeyboard );
    setEditing( false );

    m_lyricsProxy->setWidget( m_lyrics );

    QPalette pal;
    QBrush brush( PaletteHandler::highlightColor().lighter( 170 ) );
    brush.setStyle( Qt::SolidPattern );
    pal.setBrush( QPalette::Active, QPalette::Base, brush );
    pal.setBrush( QPalette::Inactive, QPalette::Base, brush );
    pal.setBrush( QPalette::Disabled, QPalette::Base, brush );
    pal.setBrush( QPalette::Window, brush );
    m_lyrics->setPalette( pal );
    m_lyricsProxy->setPalette( pal );

    // only show when we need to let the user
    // choose between suggestions
    m_suggested = new QGraphicsTextItem( this );
    connect( m_suggested, SIGNAL( linkActivated( const QString& ) ), this, SLOT( suggestionChosen( const QString& ) ) );
    m_suggested->setTextInteractionFlags( Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard );
    m_suggested->hide();
    
    connect( dataEngine( "amarok-lyrics" ), SIGNAL( sourceAdded( const QString& ) ), this, SLOT( connectSource( const QString& ) ) );
    connect( The::paletteHandler(), SIGNAL( newPalette( const QPalette& ) ), SLOT(  paletteChanged( const QPalette &  ) ) );

    constraintsEvent();
    updateConstraints();
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
    if( source == "lyrics" )
    {
        dataEngine( "amarok-lyrics" )->connectSource( source, this );
        refreshLyrics(); // get data initally
    }
    else if( source == "suggested" )
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

    // Assumes all icons are of equal width
    const int iconWidth = m_reloadIcon->size().width();

    qreal widmax = boundingRect().width() - 2 * iconWidth * 3 - 6 * standardPadding();
    QRectF rect( ( boundingRect().width() - widmax ) / 2, 0 , widmax, 15 );
    
    m_titleLabel->setScrollingText( m_titleText, rect );
    m_titleLabel->setPos( ( size().width() - m_titleLabel->boundingRect().width() ) / 2 , standardPadding() + 3 );
   
    m_reloadIcon->setPos( size().width() - iconWidth - standardPadding(), standardPadding() );
    m_reloadIcon->show();

    QPoint editIconPos( m_reloadIcon->pos().x() - standardPadding() - iconWidth, standardPadding() );
    m_editIcon->setPos( editIconPos );
    m_saveIcon->setPos( editIconPos );
    
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
            const QString sug = suggestion.toString();
            const QStringList pieces = sug.split( " - " );
            const QString link = QString( "<a href=\"%1|%2|%3\">%4 - %5</a><br>" ).arg( pieces[ 0 ] ).arg( pieces[ 1 ] ).arg( pieces[ 2 ] ).arg( pieces[ 1 ] ).arg( pieces[ 0 ] );
            html += link;
        }
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

    // draw rounded rect around title (only if not animating )
    if ( !m_titleLabel->isAnimating() )
    {
        p->save();
        p->translate(0.5, 0.5);
        drawRoundedRectAroundText( p, m_titleLabel );
        p->restore();
    }

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

void
LyricsApplet::paletteChanged( const QPalette & palette )
{
    Q_UNUSED( palette )

    if( m_lyrics )
       m_lyrics->setStyleSheet( QString( "QTextBrowser { background-color: %1; border-width: 0px; border-radius: 0px; color: %2; }" )
            .arg( PaletteHandler::highlightColor().lighter( 150 ).name() ).arg( PaletteHandler::highlightColor().darker( 400 ).name() ) );
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

    if( !curtrack || !curtrack->artist() )
        return;

    ScriptManager::instance()->notifyFetchLyrics( curtrack->artist()->name(), curtrack->name() );
}

void
LyricsApplet::editLyrics()
{
    setEditing( true );
}

void
LyricsApplet::saveLyrics()
{
    setEditing( false );
}

void
LyricsApplet::setEditing( const bool isEditing )
{
    m_lyrics->setReadOnly( !isEditing );

    const QString backgroundColor = isEditing ? 
                                        PaletteHandler::highlightColor().lighter( 150 ).name() :
                                        "white";

    m_lyrics->setStyleSheet( QString( "QTextBrowser { background-color: %1; border-width: 0px; border-radius: 0px; color: %2; }" )
        .arg( backgroundColor )
        .arg( PaletteHandler::highlightColor().darker( 400 ).name() ) );

    // If we're editing, hide and disable the edit icon
    m_editIcon->action()->setEnabled( !isEditing );
    m_editIcon->action()->setVisible( !isEditing );

    // If we're editing, show and enable the save icon
    m_saveIcon->action()->setEnabled( isEditing );
    m_saveIcon->action()->setVisible( isEditing );
}

#include "LyricsApplet.moc"

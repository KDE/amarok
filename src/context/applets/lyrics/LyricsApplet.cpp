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

#include "LyricsApplet.h"

#include "Amarok.h"
#include "Debug.h"
#include "EngineController.h"
#include "dialogs/ScriptManager.h"
#include "meta/Meta.h"
#include "Theme.h"

#include <KStandardDirs>

#include <QAction>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsProxyWidget>
#include <QTextEdit>
#include <QPainter>
#include <QPoint>

LyricsApplet::LyricsApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_titleLabel( 0 )
    , m_reloadIcon( 0 )
    , m_lyrics( 0 )
    , m_aspectRatio( 1 )
    , m_suggested( 0 )
    , m_theme( 0 )
{
    setHasConfigurationInterface( false );
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
    m_theme = new Plasma::PanelSvg( this );
    QString imagePath = KStandardDirs::locate("data", "amarok/images/web_applet_background.svg" );

    kDebug() << "Loading theme file: " << imagePath;

    m_theme->setImagePath( imagePath );
    m_theme->setContainsMultipleImages( true );
    m_theme->setEnabledBorders( Plasma::PanelSvg::AllBorders );
    
    m_titleLabel = new QGraphicsSimpleTextItem( i18n( "Lyrics" ), this );
    QFont bigger = m_titleLabel->font();
    bigger.setPointSize( bigger.pointSize() + 4 );
    m_titleLabel->setFont( bigger );
    m_titleLabel->setZValue( m_titleLabel->zValue() + 100 );
    
    // TODO add i18n when possible
    QAction* reloadAction = new QAction( "Reload Lyrics", this );
    reloadAction->setIcon( KIcon( "view-refresh" ) );
    reloadAction->setVisible( true );
    reloadAction->setEnabled( true );
    m_reloadIcon = addAction( reloadAction );

    connect( m_reloadIcon, SIGNAL( activated() ), this, SLOT( refreshLyrics() ) );
    
    m_lyricsProxy = new QGraphicsProxyWidget( this );
    m_lyrics = new QTextEdit;
    m_lyrics->setReadOnly( true );
    m_lyrics->setFrameShape( QFrame::NoFrame );
    m_lyricsProxy->setWidget( m_lyrics );

    // only show when we need to let the user
    // choose between suggestions
    m_suggested = new QGraphicsTextItem( this );
    connect( m_suggested, SIGNAL( linkActivated( const QString& ) ), this, SLOT( suggestionChosen( const QString& ) ) );
    m_suggested->setTextInteractionFlags( Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard );
    m_suggested->hide();
    
    connect( dataEngine( "amarok-lyrics" ), SIGNAL( sourceAdded( const QString& ) ), this, SLOT( connectSource( const QString& ) ) );

    constraintsEvent();
    connectSource( "lyrics" );
}

Plasma::Icon*
LyricsApplet::addAction( QAction *action )
{
    DEBUG_BLOCK
    if ( !action ) {
        debug() << "ERROR!!! PASSED INVALID ACTION";
        return 0;
    }

    Plasma::Icon *tool = new Plasma::Icon( this );

    tool->setAction( action );
    tool->setText( "" );
    tool->setToolTip( action->text() );
    tool->setDrawBackground( false );
    tool->setOrientation( Qt::Horizontal );
    QSizeF iconSize = tool->sizeFromIconSize( 22 );
    tool->setMinimumSize( iconSize );
    tool->setMaximumSize( iconSize );
    tool->resize( iconSize );


    tool->hide();
    tool->setZValue( zValue() + 1 );

    return tool;
}

void LyricsApplet::connectSource( const QString& source )
{
    DEBUG_BLOCK

    if( source == "lyrics" ) {
        dataEngine( "amarok-lyrics" )->connectSource( source, this );
        dataUpdated( source, dataEngine("amarok-lyrics" )->query( "lyrics" ) ); // get data initally
    } else if( source == "suggested" )
    {
        dataEngine( "amarok-lyrics" )->connectSource( source, this );
        dataUpdated( source, dataEngine("amarok-lyrics" )->query( "suggested" ) ); 
    }
} 

void LyricsApplet::constraintsEvent( Plasma::Constraints constraints )
{
    prepareGeometryChange();

    m_suggested->setTextWidth( size().width() );

    m_theme->resizePanel( size().toSize() );

    m_titleLabel->setPos( (size().width() - m_titleLabel->boundingRect().width() ) / 2, 0 );
    
    m_reloadIcon->setPos( QPointF( size().width() - m_reloadIcon->size().width() - 20, 0 ) );
    m_reloadIcon->show();
    
    //m_lyricsProxy->setPos( 0, m_reloadIcon->size().height() );
    QSize lyricsSize( size().width() - 20, size().height() - 24 );
    m_lyricsProxy->setMinimumSize( lyricsSize );
    m_lyricsProxy->setMaximumSize( lyricsSize );
    m_lyricsProxy->setPos( 10, 14 );
    
}

void LyricsApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    Q_UNUSED( name )
    DEBUG_BLOCK

    if( data.size() == 0 ) return;

    //debug() << "lyrics applet got name:" << name << "and lyrics: " << data;

    m_titleLabel->show();
    if( data.contains( "noscriptrunning" ) )
    {
        m_suggested->hide();
        m_lyrics->show();m_lyrics->setPlainText( i18n( "No lyrics script is running!" ) );
    } else if( data.contains( "fetching" ) )
    {
        m_suggested->hide();
        m_lyrics->show();
        m_lyrics->setPlainText( i18n( "Lyrics are being fetched." ) );
    } else if( data.contains( "error" ) )
    {
        m_suggested->hide();
        m_lyrics->show();
        m_lyrics->setPlainText( i18n( "Lyrics were not able to be downloaded. Please check your internet connection." ) );
    } else if( data.contains( "suggested" ) )
    {
        m_lyrics->hide();
        QVariantList suggested = data[ "suggested" ].toList();
        // build simple HTML to show
        // a list
        QString html = QString( "<br><br>" );
        foreach( QVariant suggestion, suggested )
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
    } else if( data.contains( "html" ) )
    {
        // show pure html in the text area
        m_suggested->hide();
        // we assume html lyrics take care of titles as well
        m_titleLabel->hide();
        m_lyrics->setHtml( data[ "html" ].toString() );
        m_lyrics->show();
    } else if( data.contains( "lyrics" ) )
    {
        m_suggested->hide();
        m_lyrics->show();
        QVariantList lyrics  = data[ "lyrics" ].toList();

        //  need padding for title
        m_lyrics->setPlainText( "\n\n" + lyrics[ 3 ].toString() );
    }
    else if( data.contains( "notfound" ) )
    {
        m_suggested->hide();
        m_lyrics->show();
        m_lyrics->setPlainText( i18n( "There were no lyrics found for this track" ) );
    }
    setPreferredSize( (int)size().width(), (int)size().height() );
}

bool LyricsApplet::hasHeightForWidth() const
{
    return false;
}

void
LyricsApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option );

    m_theme->resizePanel( size().toSize() );

    m_theme->paintPanel( p, QRectF( 0.0, 0.0, size().toSize().width(), size().toSize().height() ) );

}


#include "LyricsApplet.moc"


QSizeF LyricsApplet::sizeHint(Qt::SizeHint which, const QSizeF & constraint) const
{
    if( constraint.height() == -1 && constraint.width() > 0 ) // asking height for given width basically
    {
        return QSizeF( constraint.width(), m_aspectRatio * constraint.width() );
    } else
    {
        return constraint;
    }
}
void
LyricsApplet::suggestionChosen( const QString& link )
{
    DEBUG_BLOCK
    debug() << "got link selected:" << link;
    QStringList pieces = link.split( "|" );
    ScriptManager::instance()->notifyFetchLyricsByUrl( pieces[ 1 ], pieces[ 0 ], pieces[ 2 ] );
}

void
LyricsApplet::refreshLyrics()
{
    Meta::TrackPtr curtrack = The::engineController()->currentTrack();
    debug() << "checking for current track:";
    if( !curtrack )
        return;
    ScriptManager::instance()->notifyFetchLyrics( curtrack->artist()->name(), curtrack->name() );
}

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
#include "dialogs/ScriptManager.h"
#include "Theme.h"

#include <QGraphicsSimpleTextItem>
#include <QGraphicsProxyWidget>
#include <QTextEdit>
#include <QPainter>
#include <QPoint>

LyricsApplet::LyricsApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_lyrics( 0 )
    , m_aspectRatio( 1 )
    , m_suggested( 0 )
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
    m_lyricsProxy = new QGraphicsProxyWidget( this );
    m_lyrics = new QTextEdit;
    m_lyrics->setReadOnly( true );
    m_lyrics->setFrameShape( QFrame::NoFrame );
    m_lyricsProxy->setWidget( m_lyrics );
    m_lyrics->setPlainText( i18n( "Hello, World" ) );

    // only show when we need to let the user
    // choose between suggestions
    m_suggested = new QGraphicsTextItem( this );
    connect( m_suggested, SIGNAL( linkActivated( const QString& ) ), this, SLOT( suggestionChosen( const QString& ) ) );
    m_suggested->setTextInteractionFlags( Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard );
    m_suggested->hide();
    
    connect( dataEngine( "amarok-lyrics" ), SIGNAL( sourceAdded( const QString& ) ), this, SLOT( connectSource( const QString& ) ) );

    constraintsEvent();
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
        dataUpdated( source, dataEngine("amarok-lyrics" )->query( "lyrics" ) ); 
    }
} 

void LyricsApplet::constraintsEvent( Plasma::Constraints constraints )
{
    prepareGeometryChange();

    m_suggested->setTextWidth( size().width() );
    
    m_lyricsProxy->setMinimumSize( size() );
    m_lyricsProxy->setMaximumSize( size() );
}

void LyricsApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    Q_UNUSED( name )
    DEBUG_BLOCK

    if( data.size() == 0 ) return;

    //debug() << "lyrics applet got name:" << name << "and lyrics: " << data;

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
        QString html = QString( "<h2 align='center'>" + i18n( "Lyrics" ) + "</h2><br />" );
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
        m_lyrics->setHtml( data[ "html" ].toString() );
        m_lyrics->show();
    } else if( data.contains( "lyrics" ) )
    {
        m_suggested->hide();
        m_lyrics->show();
        QVariantList lyrics  = data[ "lyrics" ].toList();

        m_lyrics->setPlainText( lyrics[ 3 ].toString() );
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

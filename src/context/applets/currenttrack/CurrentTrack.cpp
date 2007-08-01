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

#include "CurrentTrack.h"

#include "amarok.h"
#include "debug.h"
#include "context/Svg.h"

#include <QPainter>
#include <QBrush>

CurrentTrack::CurrentTrack( QObject* parent, const QStringList& args )
    : Plasma::Applet( parent, args )
    , m_rating( -1 )
    , m_trackLength( 0 )
{
    DEBUG_BLOCK
        
    setHasConfigurationInterface( false );
    setDrawStandardBackground( false );
    
    dataEngine( "amarok-current" )->connectSource( "current", this );
    
    m_theme = new Context::Svg( "widgets/currenttrack", this );
    m_theme->setContentType( Context::Svg::SingleImage );
    m_theme->resize();
    
    m_size = m_theme->size();
    
    m_title = new QGraphicsSimpleTextItem( this );
    m_artist = new QGraphicsSimpleTextItem( this );
    m_album = new QGraphicsSimpleTextItem( this );
    m_score = new QGraphicsSimpleTextItem( this );
    m_numPlayed = new QGraphicsSimpleTextItem( this );
    m_playedLast = new QGraphicsSimpleTextItem( this );
    m_albumCover = new QGraphicsPixmapItem( this );

    m_title->setBrush( QBrush( Qt::white ) );
    m_artist->setBrush( QBrush( Qt::white ) );
    m_album->setBrush( QBrush( Qt::white ) );
    m_score->setBrush( QBrush( Qt::white ) );
    m_numPlayed->setBrush( QBrush( Qt::white ) );
    m_playedLast->setBrush( QBrush( Qt::white ) );
    
//     constraintsUpdated();
}

CurrentTrack::~CurrentTrack()
{
    delete m_title;
    delete m_artist;
    delete m_album;
    delete m_score;
    delete m_numPlayed;
    delete m_playedLast;
    delete m_albumCover;
    delete m_theme;
}

QSizeF CurrentTrack::contentSize() const
{
    return m_size;
}

void CurrentTrack::constraintsUpdated()
{
    prepareGeometryChange();
    // here we put all of the text items into the correct locations
    m_title->setPos( m_theme->elementRect( "track" ).topLeft() );
    m_artist->setPos( m_theme->elementRect( "artist" ).topLeft() );
    m_album->setPos( m_theme->elementRect( "album" ).topLeft() );
    m_score->setPos( m_theme->elementRect( "score" ).topLeft() );
    m_numPlayed->setPos( m_theme->elementRect( "numplayed" ).topLeft() );
    m_playedLast->setPos( m_theme->elementRect( "playedlast" ).topLeft() );
//     m_albumCover->setGeometry( m_theme->elementRect( "albumart" ) );
    
    update();
}

void CurrentTrack::updated( const QString& name, const Plasma::DataEngine::Data& data )
{
    DEBUG_BLOCK
    Q_UNUSED( name );
    
    QVariantList currentInfo = data[ "current" ].toList();
    kDebug() << "got data from engine: " << currentInfo << endl;
    m_title->setText( currentInfo[ 1 ].toString() );
    m_artist->setText( currentInfo[ 0 ].toString() );
    m_album->setText( currentInfo[ 2 ].toString() );
//     m_rating = currentInfo[ 3 ].toInt();
    // TODO i can't add ratings... so hardcoding to test
    m_rating = 6;
    m_score->setText( currentInfo[ 4 ].toString() );
    m_trackLength = currentInfo[ 5 ].toInt();
//     m_playedLast->setText( Amarok::verboseTimeSince( currentInfo[ 6 ].toInt() ) );
    m_numPlayed->setText( currentInfo[ 5 ].toString() );
    // TODO include albumCover when engine is fixed
}

void CurrentTrack::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option );
    
    QRect tempRect( 0, 0, 0, 0 );
    
    p->save();
    m_theme->paint( p, contentsRect, "background" );
    p->restore();
    
//     kDebug() << "first star pos: " << m_theme->elementRect( "star" ) << endl;
    
    p->save();
    int stars = m_rating / 2;
    for( int i = 1; i <= stars; i++ )
    {
        p->translate( i * m_theme->elementSize( "star" ).width(), 0 );
        tempRect = m_theme->elementRect( "star" );
        tempRect.setSize( m_theme->elementSize( "star" ) );
        m_theme->paint( p, tempRect, "star" );
    } //
    if( stars % 2 != 0 )
    { // paint a half star too
        p->translate( m_theme->elementSize( "half-star" ).width(), 0 );
        tempRect.setSize( m_theme->elementSize( "half-star" ) );
        m_theme->paint( p, tempRect, "half-star" );
    }
    p->restore();
    
    kDebug() << "contents rect: " << contentsRect << " size: " << m_theme->size() << endl;
    
    m_title->setPos( m_theme->elementRect( "track" ).topLeft() );
    m_artist->setPos( m_theme->elementRect( "artist" ).topLeft() );
    m_album->setPos( m_theme->elementRect( "album" ).topLeft() );
    m_score->setPos( m_theme->elementRect( "score" ).topLeft() );
    m_numPlayed->setPos( m_theme->elementRect( "numplayed" ).topLeft() );
    m_playedLast->setPos( m_theme->elementRect( "playedlast" ).topLeft() );
//     m_albumCover->setGeometry( m_theme->elementRect( "albumart" ) );
    
    // TODO get, and then paint, album pixmap
    
}

#include "CurrentTrack.moc"

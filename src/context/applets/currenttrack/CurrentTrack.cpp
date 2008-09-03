/*****************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>          *
 *                      : (C) 2008 William Viana Soares <vianasw@gmail.com>  *
 *****************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "CurrentTrack.h"

#include "Amarok.h"
#include "App.h"
#include "Debug.h"
#include "EngineController.h"
#include "context/popupdropper/PopupDropperAction.h"
#include "meta/CurrentTrackActionsCapability.h"
#include "meta/MetaUtility.h"
#include <context/widgets/RatingWidget.h>

#include <plasma/theme.h>

#include <KApplication>
#include <KColorScheme>
#include <KIcon>
#include <KMessageBox>

#include <QPainter>
#include <QBrush>
#include <QFont>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QMap>

CurrentTrack::CurrentTrack( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_configLayout( 0 )
    , m_width( 0 )
    , m_aspectRatio( 0.0 )
    , m_rating( -1 )
    , m_trackLength( 0 )
{
    setHasConfigurationInterface( false );
}

CurrentTrack::~CurrentTrack()
{}

void CurrentTrack::init()
{
    DEBUG_BLOCK

    m_theme = new Context::Svg( this );
    m_theme->setImagePath( "widgets/amarok-currenttrack" );
    m_theme->setContainsMultipleImages( true );
 
    m_width = globalConfig().readEntry( "width", 500 );

    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 1  );

    m_ratingWidget = new RatingWidget( this );

    connect( m_ratingWidget, SIGNAL( ratingChanged( int ) ), SLOT( changeTrackRating( int ) ) );

    m_title        = new QGraphicsSimpleTextItem( this );
    m_artist       = new QGraphicsSimpleTextItem( this );
    m_album        = new QGraphicsSimpleTextItem( this );
    m_score        = new QGraphicsSimpleTextItem( this );
    m_numPlayed    = new QGraphicsSimpleTextItem( this );
    m_playedLast   = new QGraphicsSimpleTextItem( this );
    m_noTrack      = new QGraphicsSimpleTextItem( this );
    m_albumCover   = new QGraphicsPixmapItem    ( this );

    m_score->setToolTip( i18n( "Score" ) );
    m_numPlayed->setToolTip( i18n( "Play Count" ) );
    m_playedLast->setToolTip( i18n( "Last Played" ) );
    
    QPen pen;
    pen.setBrush( ( KColorScheme( QPalette::Active ).foreground( KColorScheme::NormalText ) ) );
    pen.setStyle( Qt::SolidLine );

    m_title->setPen( pen );
    m_artist->setPen( pen );
    m_album->setPen( pen );
    m_score->setPen( pen );
    m_numPlayed->setPen( pen );
    m_playedLast->setPen( pen );
    m_noTrack->setPen( pen );


    QFont bigFont( labelFont );
    bigFont.setPointSize( bigFont.pointSize() +  2 );
    
    QFont tinyFont( labelFont );
    tinyFont.setPointSize( tinyFont.pointSize() - 4 );

    m_noTrack->setFont( bigFont );
    m_title->setFont( labelFont );
    m_artist->setFont( labelFont );
    m_album->setFont( labelFont );
    
    m_score->setFont( tinyFont );
    m_numPlayed->setFont( tinyFont );
    m_playedLast->setFont( tinyFont );

    // get natural aspect ratio, so we can keep it on resize
    m_theme->resize();
    m_aspectRatio = (qreal)m_theme->size().height() / (qreal)m_theme->size().width();
    resize( m_width, m_aspectRatio );
    
    m_noTrackText = i18n( "No track playing" );
    m_noTrack->setText( m_noTrackText );
    m_noTrack->setPen( pen );
    
    connectSource( "current" );
    connect( dataEngine( "amarok-current" ), SIGNAL( sourceAdded( const QString& ) ), this, SLOT( connectSource( const QString& ) ) );
}

void
CurrentTrack::connectSource( const QString &source )
{
    if( source == "current" )
    {
        dataEngine( "amarok-current" )->connectSource( source, this );
        dataUpdated( source, dataEngine("amarok-current" )->query( "current" ) ); // get data initally
    }
}

void CurrentTrack::changeTrackRating( int rating )
{
    DEBUG_BLOCK
    Meta::TrackPtr track = The::engineController()->currentTrack();
    track->setRating( rating );
    debug() << "change rating to: " << rating;
}

QList<QAction*>
CurrentTrack::contextualActions()
{
    QList<QAction*> actions;

    Meta::TrackPtr track = The::engineController()->currentTrack();
    
    if( !track )
        return actions;
    
    Meta::AlbumPtr album = track->album();

    if( album )
    {
        Meta::CustomActionsCapability *cac = album->as<Meta::CustomActionsCapability>();
        if( cac )
        {
            QList<PopupDropperAction *> pudActions = cac->customActions();
             
            foreach( PopupDropperAction *action, pudActions )
                actions.append( action );
        }
    }

    return actions;
}

void CurrentTrack::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints )
    DEBUG_BLOCK

    prepareGeometryChange();

    /*if( constraints & Plasma::SizeConstraint )
         m_theme->resize(size().toSize());*/

    //bah! do away with trying to get postions from an svg as this is proving wildly inaccurate
    const qreal margin = 14.0;
    const qreal albumWidth = size().toSize().height() - 2.0 * margin;
    resizeCover( m_bigCover, margin, albumWidth );

    const qreal labelX = albumWidth + margin + 14.0;
    const qreal labelWidth = 15;
    const qreal textX = labelX + labelWidth + margin;

    const qreal textHeight = ( ( size().toSize().height() - 3 * margin )  / 5.0 ) ;
    const qreal textWidth = size().toSize().width() - ( textX + margin + 23 ); // add 23 to ensure that we do not paint into the small labels
    

    // here we put all of the text items into the correct locations

    m_noTrack->setPos( size().toSize().width() / 2 - m_noTrack->boundingRect().width() / 2,
                       size().toSize().height() / 2  - 30 );
    
    m_title->setPos( QPointF( textX, margin ) );
    m_artist->setPos(  QPointF( textX, margin * 2 + textHeight ) );
    m_album->setPos( QPointF( textX, margin * 3 + textHeight * 2.0 ) );
    
    m_score->setPos( QPointF( contentsRect().width() - 15, margin + 4 ) );
    m_numPlayed->setPos( QPointF( contentsRect().width() - 14, margin + 31 ) );
    m_playedLast->setPos( QPointF( contentsRect().width() - 15, margin + 58 ) );

    const QString title = m_currentInfo[ Meta::Field::TITLE ].toString();
    const QString artist = m_currentInfo.contains( Meta::Field::ARTIST ) ? m_currentInfo[ Meta::Field::ARTIST ].toString() : QString();
    const QString album = m_currentInfo.contains( Meta::Field::ALBUM ) ? m_currentInfo[ Meta::Field::ALBUM ].toString() : QString();
    const QString lastPlayed = m_currentInfo.contains( Meta::Field::LAST_PLAYED ) ? Amarok::conciseTimeSince( m_currentInfo[ Meta::Field::LAST_PLAYED ].toUInt() ) : QString();

    const QFont textFont = shrinkTextSizeToFit( title, QRectF( 0, 0, textWidth, textHeight ) );
    const QFont labeFont = textFont;
    QFont tinyFont( textFont );

    if ( tinyFont.pointSize() > 5 ) 
        tinyFont.setPointSize( tinyFont.pointSize() - 5 );
    else
        tinyFont.setPointSize( 1 );

    m_maxTextWidth = textWidth;
    //m_maxTextWidth = size().toSize().width() - m_title->pos().x() - 14;


    m_title->setFont( textFont );
    m_artist->setFont( textFont );
    m_album->setFont( textFont );
    
    m_score->setFont( tinyFont );
    m_numPlayed->setFont( tinyFont );
    m_playedLast->setFont( tinyFont );

    m_artist->setText( truncateTextToFit( artist, m_artist->font(), QRectF( 0, 0, textWidth, 30 ) ) );
    m_title->setText( truncateTextToFit( title, m_title->font(), QRectF( 0, 0, textWidth, 30 ) ) );    
    m_album->setText( truncateTextToFit( album, m_album->font(), QRectF( 0, 0, textWidth, 30 ) ) );
    
    if( !m_noTrackText.isEmpty() )
        m_noTrack->setText( truncateTextToFit( m_noTrackText, m_noTrack->font(), QRectF( 0, 0, textWidth, 30 ) ) );

    m_ratingWidget->setPos( labelX + 25, margin * 4.0 + textHeight * 3.0 - 5.0 );
    m_ratingWidget->setMinimumSize( contentsRect().width() / 5, textHeight );
    m_ratingWidget->setMaximumSize( contentsRect().width() / 5, textHeight );
    m_ratingWidget->setSpacing( 2 );
    m_ratingWidget->show();

    dataEngine( "amarok-current" )->setProperty( "coverWidth", m_theme->elementRect( "albumart" ).size().width() );
}

void CurrentTrack::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    DEBUG_BLOCK
    Q_UNUSED( name );

    kDebug() << "CurrentTrack::dataUpdated";

    if( data.size() == 0 ) 
        return;

    QRect textRect( 0, 0, m_maxTextWidth, 30 );

    m_noTrackText = data[ "notrack" ].toString();

    if( !m_noTrackText.isEmpty() )
    {
        QRect rect( 0, 0, size().toSize().width(), 30 );
        m_noTrack->setText( truncateTextToFit( m_noTrackText, m_noTrack->font(), rect ) );
        m_noTrack->setPos( size().toSize().width() / 2 - m_noTrack->boundingRect().width() / 2,
                       size().toSize().height() / 2  - 30 );
        update();
        return;
    }

    m_noTrack->setText( QString() );
    
    m_currentInfo = data[ "current" ].toMap();
    m_title->setText( truncateTextToFit( m_currentInfo[ Meta::Field::TITLE ].toString(), m_title->font(), textRect ) );
    
    const QString artist = m_currentInfo.contains( Meta::Field::ARTIST ) ? m_currentInfo[ Meta::Field::ARTIST ].toString() : QString();
    m_artist->setText( truncateTextToFit( artist, m_artist->font(), textRect ) );
    
    const QString album = m_currentInfo.contains( Meta::Field::ALBUM ) ? m_currentInfo[ Meta::Field::ALBUM ].toString() : QString();
    m_album->setText( truncateTextToFit( album, m_album->font(), textRect ) );
    
    m_rating = m_currentInfo[ Meta::Field::RATING ].toInt();
    
    const QString score = QString::number( m_currentInfo[ Meta::Field::SCORE ].toInt() );
    m_score->setText( score );
    
    m_trackLength = m_currentInfo[ Meta::Field::LENGTH ].toInt();
    
    m_playedLast->setText( Amarok::conciseTimeSince( m_currentInfo[ Meta::Field::LAST_PLAYED ].toUInt() ) );
    m_numPlayed->setText( m_currentInfo[ Meta::Field::PLAYCOUNT ].toString() );

    m_ratingWidget->setRating( m_rating );

    //scale pixmap on demand
    //store the big cover : avoid blur when resizing the applet
    m_bigCover = data[ "albumart" ].value<QPixmap>();
//     m_sourceEmblemPixmap = data[ "source_emblem" ].value<QPixmap>();


    if( !resizeCover( m_bigCover, 14.0, size().toSize().height() - 28.0 ) )
    {
        warning() << "album cover of current track is null, did you forget to call Meta::Album::image?";
    }
    // without that the rating doesn't get update for a playing track
    update();
}


QSizeF 
CurrentTrack::sizeHint( Qt::SizeHint which, const QSizeF & constraint) const
{
    Q_UNUSED( which )

    if( constraint.height() == -1 && constraint.width() > 0 ) // asking height for given width basically
        return QSizeF( constraint.width(), 150 );
//         return QSizeF( constraint.width(), m_aspectRatio * constraint.width() );

    return constraint;
}

void CurrentTrack::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option );

    //bail out if there is no room to paint. Prevents crashes and really there is no sense in painting if the
    //context view has been minimized completely
    if( ( contentsRect.width() < 20 ) || ( contentsRect.height() < 20 ) )
    {
        foreach ( QGraphicsItem * childItem, QGraphicsItem::children() )
            childItem->hide();
        return;
    }
    else
    {
        foreach ( QGraphicsItem * childItem, QGraphicsItem::children () )
            childItem->show();        
    }
    
    p->save();
    QRect leftBorder( 0, 0, 14, contentsRect.height() + 20 );
    QRect rightBorder( contentsRect.width() + 5, 0, 14, contentsRect.height() + 20 );
    p->restore();
    
    if( !m_noTrack->text().isEmpty() )
    {
        QList<QGraphicsItem*> children = QGraphicsItem::children();
        foreach ( QGraphicsItem *childItem, children )
            childItem->hide();
        m_noTrack->show();
        return;
    }

    m_noTrack->hide();
    
    const qreal margin = 14.0;
    const qreal albumWidth = size().toSize().height() - 2.0 * margin;

    const qreal labelX = albumWidth + margin + 14.0;

    const qreal textHeight = ( ( size().toSize().height() - 3 * margin )  / 5.0 );

    p->save();
    
    
    Meta::TrackPtr track = The::engineController()->currentTrack();
    if( track && track->album()->hasImage() )
        m_theme->paint( p, QRect( margin - 5, margin, albumWidth + 12, albumWidth ), "cd-box" );

    m_theme->paint( p, QRectF( labelX, margin + 1, 16, 16 ), "track" );
    m_theme->paint( p, QRectF( labelX, margin * 2.0 + textHeight + 1 , 16, 16 ), "artist" );
    m_theme->paint( p, QRectF( labelX, margin * 3.0 + textHeight * 2.0 + 1, 16, 16 ), "album" );

    m_theme->paint( p, QRectF( contentsRect.width() - 21, margin, 23, 23 ), "score" );
    m_theme->paint( p, QRectF( contentsRect.width() - 21, margin + 26, 23, 23 ), "times-played" );
    m_theme->paint( p, QRectF( contentsRect.width() - 21, margin + 54, 23, 23 ), "last-time" );

    p->restore();

    // TODO get, and then paint, album pixmap
    // constraintsEvent();
}

void CurrentTrack::showConfigurationInterface()
{}

void CurrentTrack::configAccepted() // SLOT
{}


bool CurrentTrack::resizeCover( QPixmap cover,qreal margin, qreal width )
{
    if( !cover.isNull() )
    {
        //QSizeF rectSize = m_theme->elementRect( "albumart" ).size();
        //QPointF rectPos = m_theme->elementRect( "albumart" ).topLeft();
        qreal size = width;
        qreal pixmapRatio = (qreal)cover.width()/size;

        qreal moveByX = 0.0;
        qreal moveByY = 0.0;

        //center the cover : if the cover is not squared, we get the missing pixels and center
        if( cover.height()/pixmapRatio > width )
        {
            cover = cover.scaledToHeight( size, Qt::SmoothTransformation );
            moveByX = qAbs( cover.rect().width() - cover.rect().height() ) / 2.0;
        }
        else
        {
            cover = cover.scaledToWidth( size, Qt::SmoothTransformation );
            moveByY = qAbs( cover.rect().height() - cover.rect().width() ) / 2.0;
        }
        m_albumCover->setPos( margin + moveByX + 7.0, margin + moveByY );
//         m_sourceEmblem->setPos( margin + moveByX, margin + moveByY );

        m_albumCover->setPixmap( cover );
//         m_sourceEmblem->setPixmap( m_sourceEmblemPixmap );
        return true;
    }
    return false;
}

#include "CurrentTrack.moc"

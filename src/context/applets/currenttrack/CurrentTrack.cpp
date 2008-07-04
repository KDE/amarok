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

#include "Amarok.h"
#include "Debug.h"
#include "context/Svg.h"
#include "meta/MetaUtility.h"

#include <plasma/theme.h>

#include <KIconLoader>

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
{
}

void CurrentTrack::init()
{
    setBackgroundHints( Plasma::Applet::DefaultBackground );
    createMenu();

    m_theme = new Context::Svg( this );
    m_theme->setImagePath( "widgets/amarok-currenttrack" );
    m_theme->setContainsMultipleImages( false );
    m_width = globalConfig().readEntry( "width", 500 );

    KIconLoader *iconLoader = KIconLoader::global();

    const QColor textColor = Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor );
    QFont labelFont;
    labelFont.setBold( true );
    labelFont.setPointSize( labelFont.pointSize() + 1  );
    labelFont.setStyleHint( QFont::Times );
    labelFont.setStyleStrategy( QFont::PreferAntialias );

    QFont textFont = QFont( labelFont );
    textFont.setBold( false );

    m_titleLabel = new QGraphicsSimpleTextItem( i18nc( "The name of the playing song", "Track:" ) , this );
    m_titleLabel->setBrush( textColor );
    m_titleLabel->setFont( labelFont );

    m_artistLabel = new QGraphicsSimpleTextItem( i18n( "Artist:" ), this );
    m_artistLabel->setBrush( textColor );
    m_artistLabel->setFont( labelFont );

    m_albumLabel = new QGraphicsSimpleTextItem( i18n( "Album:" ), this );
    m_albumLabel->setBrush( textColor );
    m_albumLabel->setFont( labelFont );

    m_scoreLabel = new QGraphicsPixmapItem( QPixmap(iconLoader->loadIcon( "emblem-favorite", KIconLoader::Toolbar, KIconLoader::SizeMedium ) ), this );
    m_scoreLabel->setToolTip( i18n( "Score" ) );

    m_numPlayedLabel = new QGraphicsPixmapItem( QPixmap(iconLoader->loadIcon( "view-refresh", KIconLoader::Toolbar, KIconLoader::SizeMedium ) ), this );
    m_numPlayedLabel->setToolTip( i18n( "Play Count" ) );

    m_playedLastLabel = new QGraphicsPixmapItem( QPixmap(iconLoader->loadIcon( "user-away-extended", KIconLoader::Toolbar, KIconLoader::SizeMedium ) ), this );
    m_playedLastLabel->setToolTip( i18n( "Last Played") );

    m_scoreLabel->setTransformationMode( Qt::SmoothTransformation );
    m_numPlayedLabel->setTransformationMode( Qt::SmoothTransformation );
    m_playedLastLabel->setTransformationMode( Qt::SmoothTransformation );

    m_title        = new QGraphicsSimpleTextItem( this );
    m_artist       = new QGraphicsSimpleTextItem( this );
    m_album        = new QGraphicsSimpleTextItem( this );
    m_score        = new QGraphicsSimpleTextItem( this );
    m_numPlayed    = new QGraphicsSimpleTextItem( this );
    m_playedLast   = new QGraphicsSimpleTextItem( this );
    m_albumCover   = new QGraphicsPixmapItem( this );
    m_sourceEmblem = new QGraphicsPixmapItem( this );

    m_title->setFont( textFont );
    m_artist->setFont( textFont );
    m_album->setFont( textFont );
    m_score->setFont( textFont );
    m_numPlayed->setFont( textFont );
    m_playedLast->setFont( textFont );

    m_title->setBrush( textColor );
    m_artist->setBrush( textColor );
    m_album->setBrush( textColor );
    m_score->setBrush( textColor );
    m_numPlayed->setBrush( textColor );
    m_playedLast->setBrush( textColor );

    // get natural aspect ratio, so we can keep it on resize
    m_theme->resize();
    m_aspectRatio = (qreal)m_theme->size().height() / (qreal)m_theme->size().width();
    resize( m_width, m_aspectRatio );

    dataEngine( "amarok-current" )->connectSource( "current", this );
    dataUpdated( "current", dataEngine("amarok-current" )->query( "current" ) ); // get data initally
}

void CurrentTrack::createMenu()
{
    QAction *showCoverAction  = new QAction( i18n( "Show Fullsize" ), this );
    QAction *fetchCoverAction = new QAction( i18n( "Fetch Cover" ), this );
    QAction *unsetCoverAction = new QAction( i18n( "Unset Cover" ), this );

    connect( showCoverAction,  SIGNAL( triggered() ), this, SLOT( showItemImage() ) );
    connect( fetchCoverAction, SIGNAL( triggered() ), this, SLOT( fetchItemImage() ) );
    connect( unsetCoverAction, SIGNAL( triggered() ), this, SLOT( unsetItemImage() ) );

    m_contextActions.append( showCoverAction );
    m_contextActions.append( fetchCoverAction );
    m_contextActions.append( unsetCoverAction );
}

void CurrentTrack::constraintsEvent( Plasma::Constraints constraints )
{
    prepareGeometryChange();

    /*if( constraints & Plasma::SizeConstraint )
         m_theme->resize(size().toSize());*/

    //bah! do away with trying to get postions from an svg as this is proving wildly inaccurate
    qreal margin = 4.0;
    qreal albumWidth = size().toSize().height() - 2.0 * margin;
    resizeCover( m_bigCover, margin, albumWidth );

    qreal labelX = albumWidth + 2.0 * margin;
    qreal labelWidth = size().toSize().width() / 6.0;
    qreal textX = labelX + labelWidth + margin;

    qreal textHeight = ( ( size().toSize().height() - 6 * margin )  / 5.0 ) ;
    qreal textWidth = size().toSize().width() - ( textX + margin );

    // here we put all of the text items into the correct locations
    m_titleLabel->setPos( QPointF( labelX, margin ) );
    m_artistLabel->setPos( QPointF( labelX, margin * 2 + textHeight ) );
    m_albumLabel->setPos( QPointF( labelX, margin * 3 + textHeight * 2.0 ) );
    m_title->setPos( QPointF( textX, margin ) );
    m_artist->setPos(  QPointF( textX, margin * 2 + textHeight ) );
    m_album->setPos( QPointF( textX, margin * 3 + textHeight * 2.0 ) );



    int runningX = labelX;
    
    m_scoreLabel->setPos( QPointF( runningX, margin * 5 + textHeight * 4.0 ) );
    runningX += textHeight + margin;
    m_score->setPos( QPointF( runningX, margin * 5 + textHeight * 4.0 ) );
    runningX += size().toSize().width() / 10.0;

    m_numPlayedLabel->setPos( QPointF( runningX, margin * 5 + textHeight * 4.0 ) );
    runningX += textHeight + margin;
    m_numPlayed->setPos( QPointF( runningX, margin * 5 + textHeight * 4.0 ) );
    runningX += size().toSize().width() / 10.0;
    
    m_playedLastLabel->setPos( QPointF( runningX, margin * 5 + textHeight * 4.0 ) );
    runningX += textHeight + margin;
    m_playedLast->setPos( QPointF( runningX, margin * 5 + textHeight * 4.0 ) );
   
    m_sourceEmblem->setPos( QPointF( margin, margin ) );


    QString title = m_currentInfo[ Meta::Field::TITLE ].toString();
    QString artist = m_currentInfo.contains( Meta::Field::ARTIST ) ? m_currentInfo[ Meta::Field::ARTIST ].toString() : QString();
    QString album = m_currentInfo.contains( Meta::Field::ALBUM ) ? m_currentInfo[ Meta::Field::ALBUM ].toString() : QString();
    QString lastPlayed = m_currentInfo.contains( Meta::Field::LAST_PLAYED ) ? Amarok::verboseTimeSince( m_currentInfo[ Meta::Field::LAST_PLAYED ].toUInt() ) : QString();

    QFont textFont = shrinkTextSizeToFit( title, QRectF( 0, 0, textWidth, textHeight ) );
    QFont labeFont = textFont;
    labeFont.setBold( true );
    
    m_maxTextWidth = size().toSize().width() - m_title->pos().x();

    m_titleLabel->setFont( labeFont );
    m_artistLabel->setFont( labeFont );
    m_albumLabel->setFont( labeFont );
    m_title->setFont( textFont );
    m_artist->setFont( textFont );
    m_album->setFont( textFont );
    m_score->setFont( textFont );
    m_numPlayed->setFont( textFont );
    m_playedLast->setFont( textFont );


    m_title->setText( truncateTextToFit( title, m_title->font(), QRectF( 0, 0, textWidth, 30 ) ) );
    m_artist->setText( truncateTextToFit( artist, m_artist->font(), QRectF( 0, 0, textWidth, 30 ) ) );
    m_album->setText( truncateTextToFit( album, m_album->font(), QRectF( 0, 0, textWidth, 30 ) ) );

    m_playedLast->setText( truncateTextToFit( lastPlayed, m_playedLast->font(), QRectF( 0, 0, size().toSize().width() - runningX, 30 ) ) );
   
    //calc scale factor..
    m_scoreLabel->resetTransform();
    m_numPlayedLabel->resetTransform();
    m_playedLastLabel->resetTransform();

    qreal currentHeight = m_scoreLabel->boundingRect().height();
    qreal desiredHeight = textHeight;
    qreal scaleFactor = desiredHeight / currentHeight;

    m_scoreLabel->scale( scaleFactor, scaleFactor );
    m_numPlayedLabel->scale( scaleFactor, scaleFactor );
    m_playedLastLabel->scale( scaleFactor, scaleFactor );



    dataEngine( "amarok-current" )->setProperty( "coverWidth", m_theme->elementRect( "albumart" ).size().width() );
}

void CurrentTrack::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    DEBUG_BLOCK
    Q_UNUSED( name );

    kDebug() << "CurrentTrack::dataUpdated";

    if( data.size() == 0 ) return;

    m_currentInfo = data[ "current" ].toMap();
    m_title->setText( truncateTextToFit( m_currentInfo[ Meta::Field::TITLE ].toString(), m_title->font(), m_theme->elementRect( "track" ) ) );
    QString artist = m_currentInfo.contains( Meta::Field::ARTIST ) ? m_currentInfo[ Meta::Field::ARTIST ].toString() : QString();
    m_artist->setText( truncateTextToFit( artist, m_artist->font(), m_theme->elementRect( "artist" ) ) );
    QString album = m_currentInfo.contains( Meta::Field::ALBUM ) ? m_currentInfo[ Meta::Field::ALBUM ].toString() : QString();
    m_album->setText( truncateTextToFit( album, m_album->font(), m_theme->elementRect( "album" ) ) );
    m_rating = m_currentInfo[ Meta::Field::RATING ].toInt();
    m_score->setText( QString::number( m_currentInfo[ Meta::Field::SCORE ].toInt() ) );
    m_trackLength = m_currentInfo[ Meta::Field::LENGTH ].toInt();
    m_playedLast->setText( Amarok::verboseTimeSince( m_currentInfo[ Meta::Field::LAST_PLAYED ].toUInt() ) );
    m_numPlayed->setText( m_currentInfo[ Meta::Field::PLAYCOUNT ].toString() );

    //scale pixmap on demand
    //store the big cover : avoid blur when resizing the applet
    m_bigCover = data[ "albumart" ].value<QPixmap>();
    m_sourceEmblemPixmap = data[ "source_emblem" ].value<QPixmap>();


    if( !resizeCover( m_bigCover, 4.0, size().toSize().height() - 8.0 ) )
    {
        warning() << "album cover of current track is null, did you forget to call Meta::Album::image?";
    }
    // without that the rating doesn't get update for a playing track
    update();
}


QSizeF 
CurrentTrack::sizeHint( Qt::SizeHint which, const QSizeF & constraint) const
{
    if( constraint.height() == -1 && constraint.width() > 0 ) // asking height for given width basically
    {
        return QSizeF( constraint.width(), m_aspectRatio * constraint.width() );
    } else
    {
        return constraint;
    }
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
    m_theme->paint( p, contentsRect, "background" );
    p->restore();

    p->save();
    int stars = m_rating / 2;
    for( int i = 1; i <= stars; i++ )
    {
        p->translate( m_theme->elementSize( "star" ).width(), 0 );
        m_theme->paint( p, m_theme->elementRect( "star" ), "star" );
    } //
    if( m_rating % 2 != 0 )
    { // paint a half star too
        m_theme->paint( p, m_theme->elementRect( "half-star" ), "half-star" );
    }
    p->restore();

    // TODO get, and then paint, album pixmap
//     constraintsEvent();

}

void CurrentTrack::showConfigurationInterface()
{
}

void CurrentTrack::configAccepted() // SLOT
{
}


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
        m_albumCover->setPos( margin + moveByX, margin + moveByY );
        m_sourceEmblem->setPos( margin + moveByX, margin + moveByY );

        m_albumCover->setPixmap( cover );
        m_sourceEmblem->setPixmap( m_sourceEmblemPixmap );
        return true;
    }
    return false;
}

#include "CurrentTrack.moc"

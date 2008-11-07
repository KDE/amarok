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

#include "Songkick.h"

#include "Amarok.h"
#include "Debug.h"
#include "EngineController.h"
#include "context/popupdropper/PopupDropperAction.h"
#include "meta/MetaUtility.h"
#include "PaletteHandler.h"
#include "SvgHandler.h"
#include <context/widgets/RatingWidget.h>

#include <plasma/theme.h>

#include <KApplication>
#include <KColorScheme>
#include <KIcon>
#include <KMessageBox>

#include <QCheckBox>
#include <QFont>
#include <QLabel>
#include <QPainter>
#include <QSpinBox>
#include <QVBoxLayout>


Songkick::Songkick( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_configLayout( 0 )
    , m_width( 0 )
    , m_aspectRatio( 0.0 )
    , m_rating( -1 )
    , m_trackLength( 0 )
    , m_tracksToShow( 0 )
{
    setHasConfigurationInterface( false );
}

Songkick::~Songkick()
{}

void Songkick::init()
{
    DEBUG_BLOCK

    m_theme = new Context::Svg( this );
    m_theme->setImagePath( "widgets/amarok-songkick" );
    m_theme->setContainsMultipleImages( true );
 
    m_width = globalConfig().readEntry( "width", 500 );

    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 1  );

    m_title        = new QGraphicsSimpleTextItem( this );
    m_artist       = new QGraphicsSimpleTextItem( this );
    m_album        = new QGraphicsSimpleTextItem( this );
    m_score        = new QGraphicsSimpleTextItem( this );
    m_numPlayed    = new QGraphicsSimpleTextItem( this );
    m_playedLast   = new QGraphicsSimpleTextItem( this );
    m_noTrack      = new QGraphicsSimpleTextItem( this );
    m_albumCover   = new QGraphicsPixmapItem    ( this );

    m_score->setToolTip( i18n( "Score:" ) );
    m_numPlayed->setToolTip( i18n( "Play Count:" ) );
    m_playedLast->setToolTip( i18nc("a single item (singular)", "Last Played:") );
    

    QBrush brush = KColorScheme( QPalette::Active ).foreground( KColorScheme::NormalText );

    m_title->setBrush( brush );
    m_artist->setBrush( brush );
    m_album->setBrush( brush );
    m_score->setBrush( brush );
    m_numPlayed->setBrush( brush );
    m_playedLast->setBrush( brush );
    m_noTrack->setBrush( brush );

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
    m_noTrack->hide();
    m_noTrack->setText( m_noTrackText );


    connectSource( "dates" );
    connect( dataEngine( "amarok-songkick" ), SIGNAL( sourceAdded( const QString& ) ), this, SLOT( connectSource( const QString& ) ) );
    connect( The::paletteHandler(), SIGNAL( newPalette( const QPalette& ) ), SLOT(  paletteChanged( const QPalette &  ) ) );
}

void
Songkick::connectSource( const QString &source )
{
    if( source == "dates" )
    {
        dataEngine( "amarok-songkick" )->connectSource( source, this );
        dataUpdated( source, dataEngine("amarok-songkick" )->query( "dates" ) ); // get data initally
    }
}

void Songkick::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints )
    DEBUG_BLOCK

    prepareGeometryChange();

    /*if( constraints & Plasma::SizeConstraint )
         m_theme->resize(size().toSize());*/

    //bah! do away with trying to get postions from an svg as this is proving wildly inaccurate
    const qreal margin = 14.0;
    const qreal albumWidth = size().toSize().height() - 2.0 * margin;

    const qreal labelX = albumWidth + margin + 14.0;
    const qreal labelWidth = 15;
    const qreal textX = labelX + labelWidth + margin;

    const qreal textHeight = ( ( size().toSize().height() - 3 * margin )  / 5.0 ) ;
    const qreal textWidth = size().toSize().width() - ( textX + margin + 53 ); // add 53 to ensure room for small label + their text
    

    // here we put all of the text items into the correct locations    
    
    m_title->setPos( QPointF( textX, margin + 2 ) );
    m_artist->setPos(  QPointF( textX, margin * 2 + textHeight + 2  ) );
    m_album->setPos( QPointF( textX, margin * 3 + textHeight * 2.0 + 2 ) );

    const int addLabelOffset = contentsRect().width() - 25;
    
    m_score->setPos( QPointF( addLabelOffset, margin + 2 ) );
    m_numPlayed->setPos( QPointF( addLabelOffset, margin * 2 + textHeight + 2 ) );
    m_playedLast->setPos( QPointF( addLabelOffset, margin * 3 + textHeight * 2.0 + 2 ) );

    const QString title = m_currentInfo[ Meta::Field::TITLE ].toString();
    const QString artist = m_currentInfo.contains( Meta::Field::ARTIST ) ? m_currentInfo[ Meta::Field::ARTIST ].toString() : QString();
    const QString album = m_currentInfo.contains( Meta::Field::ALBUM ) ? m_currentInfo[ Meta::Field::ALBUM ].toString() : QString();
    const QString lastPlayed = m_currentInfo.contains( Meta::Field::LAST_PLAYED ) ? Amarok::conciseTimeSince( m_currentInfo[ Meta::Field::LAST_PLAYED ].toUInt() ) : QString();

    const QFont textFont = shrinkTextSizeToFit( title, QRectF( 0, 0, textWidth, textHeight ) );
    const QFont labeFont = textFont;
    QFont tinyFont( textFont );

    if ( tinyFont.pointSize() > 7 )
        tinyFont.setPointSize( tinyFont.pointSize() - 2 );
    else
        tinyFont.setPointSize( 5 );

    m_maxTextWidth = textWidth;
    //m_maxTextWidth = size().toSize().width() - m_title->pos().x() - 14;


    m_title->setFont( textFont );
    m_artist->setFont( textFont );
    m_album->setFont( textFont );
    
    m_score->setFont( textFont );
    m_numPlayed->setFont( textFont );
    m_playedLast->setFont( textFont );

    m_artist->setText( truncateTextToFit( artist, m_artist->font(), QRectF( 0, 0, textWidth, 30 ) ) );
    m_title->setText( truncateTextToFit( title, m_title->font(), QRectF( 0, 0, textWidth, 30 ) ) );    
    m_album->setText( truncateTextToFit( album, m_album->font(), QRectF( 0, 0, textWidth, 30 ) ) );
    
    if( !m_tracks.isEmpty() )
    {
        m_noTrack->setText( truncateTextToFit( i18n( "Last Played" ), m_noTrack->font(), QRectF( 0, 0, textWidth, 30 ) ) );
        QFontMetricsF fm( m_noTrack->font() );
                
        m_noTrack->setPos( size().toSize().width() / 2 - m_noTrack->boundingRect().width() / 2, 10 );

    }        
    else if( !m_noTrackText.isEmpty() )
    {
        m_noTrack->setText( truncateTextToFit( m_noTrackText, m_noTrack->font(), QRectF( 0, 0, textWidth, 30 ) ) );
        m_noTrack->setPos( size().toSize().width() / 2 - m_noTrack->boundingRect().width() / 2,
                       size().toSize().height() / 2  - 30 );
    }

    dataEngine( "amarok-current" )->setProperty( "coverWidth", m_theme->elementRect( "albumart" ).size().width() );
}

void Songkick::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    DEBUG_BLOCK
    Q_UNUSED( name );

    kDebug() << "Songkick::dataUpdated";

    if( data.size() == 0 ) 
        return;

    QRect textRect( 0, 0, m_maxTextWidth, 30 );

    m_noTrackText = data[ "dates" ].toString();
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
    m_tracks.clear();
    
    const QString artist = data[ "artist" ].toString();
   
    m_artist->setText( truncateTextToFit( artist, m_artist->font(), textRect ) );
    
    update();
}


QSizeF 
Songkick::sizeHint( Qt::SizeHint which, const QSizeF & constraint) const
{
    Q_UNUSED( which )

    if( constraint.height() == -1 && constraint.width() > 0 ) // asking height for given width basically
        return QSizeF( constraint.width(), 150 );
//         return QSizeF( constraint.width(), m_aspectRatio * constraint.width() );

    return constraint;
}

void Songkick::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
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

    if( !m_tracks.isEmpty() )
    {
        QList<QGraphicsItem*> children = QGraphicsItem::children();
        foreach ( QGraphicsItem *childItem, children )
            childItem->hide();
        m_noTrack->show();
        return;
    }
    else if( !m_noTrack->text().isEmpty() )
    {
        QList<QGraphicsItem*> children = QGraphicsItem::children();
        foreach ( QGraphicsItem *childItem, children )
            childItem->hide();
        m_noTrack->show();
        return;
    }
    else
    {
        m_noTrack->hide();
    }
    
    const qreal margin = 14.0;
    const qreal albumWidth = size().toSize().height() - 2.0 * margin;

    const qreal labelX = albumWidth + margin + 14.0;

    const qreal textHeight = ( ( size().toSize().height() - 3 * margin )  / 5.0 );

    p->save();
    
    
    Meta::TrackPtr track = The::engineController()->currentTrack();

    //don't paint this until we have something better looking that also works with non square covers
    /*if( track && track->album() && track->album()->hasImage() )
        m_theme->paint( p, QRect( margin - 5, margin, albumWidth + 12, albumWidth ), "cd-box" );*/

    const int lineSpacing = margin + textHeight;
    const int line1Y = margin + 1;
    const int line2Y = line1Y + lineSpacing;
    const int line3Y = line2Y + lineSpacing;
    
    m_theme->paint( p, QRectF( labelX, line1Y, 23, 23 ), "track_png_23" );
    m_theme->paint( p, QRectF( labelX, line2Y , 23, 23 ), "artist_png_23" );
    m_theme->paint( p, QRectF( labelX, line3Y, 23, 23 ), "album_png_23" );


    const int label2X = contentsRect.width() - 53;
    
    m_theme->paint( p, QRectF( label2X, line1Y, 23, 23 ), "score_png_23" );
    m_theme->paint( p, QRectF( label2X, line2Y, 23, 23 ), "times-played_png_23" );
    m_theme->paint( p, QRectF( label2X, line3Y, 23, 23 ), "last-time_png_23" );

    p->restore();

    // TODO get, and then paint, album pixmap
    // constraintsEvent();
}

void Songkick::showConfigurationInterface()
{}

void Songkick::configAccepted() // SLOT
{}


void Songkick::paletteChanged( const QPalette & palette )
{
    DEBUG_BLOCK

    m_title->setBrush( palette.text() );
    m_artist->setBrush( palette.text() );
    m_album->setBrush( palette.text() );
    m_score->setBrush( palette.text() );
    m_numPlayed->setBrush( palette.text() );
    m_playedLast->setBrush( palette.text() );
    m_noTrack->setBrush( palette.text() );
}

#include "Songkick.moc"

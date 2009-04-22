/*****************************************************************************
 * copyright            : (C) 2007-2009 Leo Franchi <lfranchi@gmail.com>     *
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
#include "context/popupdropper/libpud/PopupDropperAction.h"
#include "meta/capabilities/CurrentTrackActionsCapability.h"
#include "meta/MetaUtility.h"
#include "PaletteHandler.h"
#include "SvgHandler.h"
#include <context/widgets/RatingWidget.h>

#include <plasma/theme.h>
#include <plasma/widgets/tabbar.h>

#include <KColorScheme>

#include <QFont>
#include <QGraphicsLinearLayout>
#include <QPainter>


CurrentTrack::CurrentTrack( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_rating( -1 )
    , m_trackLength( 0 )
    , m_tracksToShow( 0 )
    , m_tabBar( 0 )
{
    setHasConfigurationInterface( false );
    setConfigurationRequired( false );
    setBackgroundHints( Plasma::Applet::NoBackground );
    setImmutability( Plasma::Mutable );
}

CurrentTrack::~CurrentTrack()
{}

void CurrentTrack::init()
{
    DEBUG_BLOCK

    m_ratingWidget = new RatingWidget( this );
    m_ratingWidget->setSpacing( 2 );

    connect( m_ratingWidget, SIGNAL( ratingChanged( int ) ), SLOT( changeTrackRating( int ) ) );

    m_title        = new QGraphicsSimpleTextItem( this );
    m_artist       = new QGraphicsSimpleTextItem( this );
    m_album        = new QGraphicsSimpleTextItem( this );
    m_noTrack      = new QGraphicsSimpleTextItem( this );
    m_albumCover   = new QGraphicsPixmapItem    ( this );
    m_byText       = new QGraphicsSimpleTextItem( i18nc( "What artist is this track by", "By" ), this );
    m_onText       = new QGraphicsSimpleTextItem( i18nc( "What album is this track on", "On" ), this );

    QBrush brush = KColorScheme( QPalette::Active ).foreground( KColorScheme::NormalText );

    m_title->setBrush( brush );
    m_artist->setBrush( brush );
    m_album->setBrush( brush );;
    m_noTrack->setBrush( brush );

    QFont bigFont;
    bigFont.setPointSize( bigFont.pointSize() +  3 );
    bigFont.setBold( true );
    
    QFont tinyFont;
    tinyFont.setPointSize( tinyFont.pointSize() - 2 );

    m_noTrack->setFont( bigFont );
    m_title->setFont( bigFont );
    m_artist->setFont( bigFont );
    m_album->setFont( bigFont );
    m_byText->setFont( tinyFont );
    m_onText->setFont( tinyFont );
    
    m_noTrackText = i18n( "No track playing" );
    m_noTrack->hide();
    m_noTrack->setText( m_noTrackText );
    
    m_tabBar = new Plasma::TabBar( this );

    m_margin = 7;
    m_playCountLabel = i18n( "Play count" );
    m_scoreLabel = i18n( "Score" );
    m_lastPlayedLabel = i18n( "Last Played" );

    for( int i = 0; i < MAX_PLAYED_TRACKS; i++ )
        m_tracks[i] = new TrackWidget( this );
 
    m_tabBar->addTab( i18n( "Last played" ) );
    m_tabBar->addTab( i18n( "Favorite tracks" ) );

    // Note: TabBar disabled for 2.1-beta1 release, due to issues with visual appearance and usability
    m_tabBar->hide();

    connectSource( "current" );
    connect( m_tabBar, SIGNAL( currentChanged( int ) ), this, SLOT( tabChanged( int ) ) );
    connect( dataEngine( "amarok-current" ), SIGNAL( sourceAdded( const QString& ) ), this, SLOT( connectSource( const QString& ) ) );
    connect( The::paletteHandler(), SIGNAL( newPalette( const QPalette& ) ), SLOT(  paletteChanged( const QPalette &  ) ) );

    // completely arbitrary yet necessary to kick start the layout before the user acts and resizes manually
    resize( 500, .75 );

    // hide the items while we startup. as soon as the query is done, they'll be shown.
    foreach ( QGraphicsItem * childItem, QGraphicsItem::children() )
        childItem->hide();
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
    DEBUG_BLOCK
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

    // these all used to be based on fancy calculatons based on the height
    // guess what: the height is fixed. so that's a waste of hard-working gerbils
    const qreal textHeight = 30;
    const qreal albumWidth = 130;
    const QPointF albumCoverPos( m_margin + 10, m_margin + 2 );

    resizeCover( m_bigCover, albumWidth, albumCoverPos );

    /* this is the pos of the albumcover */
    const qreal textX =  albumCoverPos.x() +  albumWidth + m_margin;
    const qreal textWidth = size().toSize().width() - ( textX + m_margin * 2 + 23 );
    const qreal textY = albumCoverPos.y() + 20;

    // calculate font sizes
    QFontMetrics fm( m_title->font() );
    qreal lineSpacing = fm.height() + 4;
    m_maxTextWidth = textWidth;

    // align to either the album or artist line, depending of "On" or "By" is longer in this current translation
    // i10n makes things complicated :P
    if( m_byText->boundingRect().width() > m_onText->boundingRect().width() )
    { // align to location of on text
        m_byText->setPos( textX, textY + lineSpacing );
        m_artist->setPos( m_byText->pos().x() + m_byText->boundingRect().width() + 5, 0 );
        alignBottomToFirst( m_byText, m_artist );
        m_album->setPos( m_artist->pos().x(), 0 );
        m_onText->setPos( m_album->pos().x() - m_onText->boundingRect().width() - 5, textY + fm.height() + lineSpacing );
        alignBottomToFirst( m_onText, m_album );

    } else // align to location/width of by text
    {
        m_onText->setPos( textX, textY + fm.height() + lineSpacing );
        m_album->setPos( m_onText->pos().x() + m_onText->boundingRect().width() + 5, 0 );
        alignBottomToFirst( m_onText, m_album );
        m_artist->setPos( m_album->pos().x(), 0 );
        m_byText->setPos( m_artist->pos().x() - m_byText->boundingRect().width() - 5, textY + lineSpacing );
        alignBottomToFirst( m_byText, m_artist );
    }
    
    m_title->setPos( m_artist->pos().x(), textY );

    const QString title = m_currentInfo[ Meta::Field::TITLE ].toString();
    const QString artist = m_currentInfo.contains( Meta::Field::ARTIST ) ? m_currentInfo[ Meta::Field::ARTIST ].toString() : QString();
    const QString album = m_currentInfo.contains( Meta::Field::ALBUM ) ? m_currentInfo[ Meta::Field::ALBUM ].toString() : QString();
    const QString lastPlayed = m_currentInfo.contains( Meta::Field::LAST_PLAYED ) ? Amarok::conciseTimeSince( m_currentInfo[ Meta::Field::LAST_PLAYED ].toUInt() ) : QString();

    m_artist->setText( truncateTextToFit( artist, m_artist->font(), QRectF( 0, 0, textWidth, 30 ) ) );
    m_title->setText( truncateTextToFit( title, m_title->font(), QRectF( 0, 0, textWidth, 30 ) ) );
    m_album->setText( truncateTextToFit( album, m_album->font(), QRectF( 0, 0, textWidth, 30 ) ) );


    if( !m_lastTracks.isEmpty() )
    {                
        m_tracksToShow = qMin( m_lastTracks.count(), ( int )( ( contentsRect().height() ) / ( textHeight ) ) );

        // Note: TabBar disabled for 2.1-beta1 release, due to issues with visual appearance and usability
#if 0
        m_tracksToShow = qMin( m_lastTracks.count(), ( int )( ( contentsRect().height() - 30 ) / ( textHeight * 1.2 ) ) );
        QFontMetrics fm( m_tabBar->font() );
        m_tabBar->resize( QSizeF( contentsRect().width() - m_margin * 2 - 2, m_tabBar->size().height() * 0.7 ) ); // Why is the height factor ignored?
        m_tabBar->setPos( size().width() / 2 - m_tabBar->size().width() / 2 - 1, 10 );

        m_tabBar->show();
#endif
        
        for( int i = 0; i < m_tracksToShow; i++ )
        {
            m_tracks[i]->resize( contentsRect().width() - m_margin * 2, textHeight * .8 );

            // Note: TabBar disabled for 2.1-beta1 release, due to issues with visual appearance and usability
            //m_tracks[i]->setPos( ( rect().width() - m_tracks[i]->boundingRect().width() ) / 2, textHeight * 1.2 * i + 43 );
            m_tracks[i]->setPos( ( rect().width() - m_tracks[i]->boundingRect().width() ) / 2, ( textHeight * .8 + m_margin / 2 ) * i + 25 );
        }
    }        
    else if( !m_noTrackText.isEmpty() )
    {
        m_noTrack->setText( truncateTextToFit( m_noTrackText, m_noTrack->font(), QRectF( 0, 0, textWidth, 30 ) ) );
        m_noTrack->setPos( size().toSize().width() / 2 - m_noTrack->boundingRect().width() / 2,
                       size().toSize().height() / 2  - 30 );
    }

    m_ratingWidget->setMinimumSize( albumWidth + 10, textHeight );
    m_ratingWidget->setMaximumSize( albumWidth + 10, textHeight );
    
    //place directly above the bottom of the applet
    const qreal x = albumCoverPos.x() - 5;
    const qreal y = boundingRect().height() - m_ratingWidget->boundingRect().height() - m_margin;
    m_ratingWidget->setPos( x, y );
    
    dataEngine( "amarok-current" )->setProperty( "coverWidth", albumWidth );
}

void CurrentTrack::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    DEBUG_BLOCK
    Q_UNUSED( name );

    if( data.size() == 0 ) 
        return;

    QRect textRect( 0, 0, m_maxTextWidth, 30 );

    m_noTrackText = data[ "notrack" ].toString();
    m_lastTracks = data[ "lastTracks" ].value<Meta::TrackList>();
    m_favoriteTracks = data[ "favoriteTracks" ].value<Meta::TrackList>();
    
    if( !m_lastTracks.isEmpty() )
    {
        Meta::TrackList tracks;
        if( m_tabBar->currentIndex() == 0 )
            tracks = m_lastTracks;
        else
            tracks = m_favoriteTracks;
        int i = 0;
        foreach( Meta::TrackPtr track, tracks )
        {
            m_tracks[i]->setTrack( track );
            i++;
        }
        
        updateConstraints();
        update();
        return;
    }
    else if( !m_noTrackText.isEmpty() )
    {
        QRect rect( 0, 0, size().toSize().width(), 30 );
        m_noTrack->setText( truncateTextToFit( m_noTrackText, m_noTrack->font(), rect ) );
        m_noTrack->setPos( size().toSize().width() / 2 - m_noTrack->boundingRect().width() / 2,
                       size().toSize().height() / 2  - 30 );
        update();
        return;
    }

    m_noTrack->setText( QString() );
    m_lastTracks.clear();
    m_favoriteTracks.clear();

    m_currentInfo = data[ "current" ].toMap();
    m_title->setText( truncateTextToFit( m_currentInfo[ Meta::Field::TITLE ].toString(), m_title->font(), textRect ) );

    const QString artist = m_currentInfo.contains( Meta::Field::ARTIST ) ? m_currentInfo[ Meta::Field::ARTIST ].toString() : QString();
    m_artist->setText( truncateTextToFit( artist, m_artist->font(), textRect ) );

    const QString album = m_currentInfo.contains( Meta::Field::ALBUM ) ? m_currentInfo[ Meta::Field::ALBUM ].toString() : QString();
    m_album->setText( truncateTextToFit( album, m_album->font(), textRect ) );

    m_rating = m_currentInfo[ Meta::Field::RATING ].toInt();

    m_trackLength = m_currentInfo[ Meta::Field::LENGTH ].toInt();

    m_score = QString::number( m_currentInfo[ Meta::Field::SCORE ].toInt() );
    m_playedLast = Amarok::verboseTimeSince( m_currentInfo[ Meta::Field::LAST_PLAYED ].toUInt() );
    m_numPlayed  = m_currentInfo[ Meta::Field::PLAYCOUNT ].toString();

    m_ratingWidget->setRating( m_rating );

    //scale pixmap on demand
    //store the big cover : avoid blur when resizing the applet
    m_bigCover = data[ "albumart" ].value<QPixmap>();
    m_sourceEmblemPixmap = data[ "source_emblem" ].value<QPixmap>();

    // without that the rating doesn't get update for a playing track
    update();
    updateConstraints();
}

QSizeF 
CurrentTrack::sizeHint( Qt::SizeHint which, const QSizeF & constraint) const
{
    Q_UNUSED( which )

    if( constraint.height() == -1 && constraint.width() > 0 ) // asking height for given width basically
        return QSizeF( constraint.width(), 180 );
//         return QSizeF( constraint.width(), m_aspectRatio * constraint.width() );

    return constraint;
}

void CurrentTrack::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option );
    p->setRenderHint( QPainter::Antialiasing );

    // tint the whole applet
    addGradientToAppletBackground( p );
    
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

    if( !m_lastTracks.isEmpty() )
    {
        QList<QGraphicsItem*> children = QGraphicsItem::children();
        foreach ( QGraphicsItem *childItem, children )
            childItem->hide();

        // Note: TabBar disabled for 2.1-beta1 release, due to issues with visual appearance and usability
#if 0
        m_tabBar->show();
#endif 
        for( int i = 0; i < m_tracksToShow; i++)
            m_tracks[i]->show();
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
        m_tabBar->hide();
        m_noTrack->hide();
        for( int i = 0; i < MAX_PLAYED_TRACKS; i++)
            m_tracks[i]->hide();
    }
    
    p->save();
    
    
    Meta::TrackPtr track = The::engineController()->currentTrack();

    // Only show the ratings widget if the current track is in the collection
    if( track && track->inCollection() )
        m_ratingWidget->show();
    else
        m_ratingWidget->hide();

    p->restore();
    
    // draw border around ratingwidget
    p->save();
    p->setRenderHint( QPainter::Antialiasing );
    QColor fillColor( 255, 255, 255, 90 );
    QRectF borderRect = m_ratingWidget->boundingRect();
    borderRect.moveTo( m_ratingWidget->pos() ) ;
    QPainterPath toFill;
    toFill.addRoundedRect( borderRect, 5, 5 );
    //p->fillPath( toFill, fillColor );
    p->restore();

    p->save();
    qreal leftEdge = qMax( m_onText->pos().x(), m_byText->pos().x() );
    qreal localMaxTextWidth = m_maxTextWidth + qMax( m_onText->boundingRect().width(), m_byText->boundingRect().width() ) + 5;
    QColor topColor( 255, 255, 255, 120 );
    QColor bottomColor( 255, 255, 255, 90 );
    // draw the complete outline. lots of little steps :)
    // at each corner, leave a 6x6 box. draw a quad bezier curve from the two ends of the lines, through  the original corner
    QPainterPath statsPath;
    statsPath.moveTo( leftEdge + 6, m_ratingWidget->pos().y() - m_ratingWidget->boundingRect().height() + 8 ); // top left position of the rect, right below the album
    statsPath.lineTo( leftEdge + localMaxTextWidth - 6, m_ratingWidget->pos().y() - m_ratingWidget->boundingRect().height() + 8 ); // go right to margin
    statsPath.quadTo( leftEdge + localMaxTextWidth, m_ratingWidget->pos().y() - m_ratingWidget->boundingRect().height() + 8,
                    leftEdge + localMaxTextWidth, m_ratingWidget->pos().y() - m_ratingWidget->boundingRect().height() + 8 + 6 );
    statsPath.lineTo( leftEdge + localMaxTextWidth, m_ratingWidget->pos().y() + m_ratingWidget->boundingRect().height() - 6 ); // go down to bottom ight corner
    statsPath.quadTo( leftEdge + localMaxTextWidth, m_ratingWidget->pos().y() + m_ratingWidget->boundingRect().height(),
                    leftEdge + localMaxTextWidth - 6, m_ratingWidget->pos().y() + m_ratingWidget->boundingRect().height() );
    statsPath.lineTo( m_ratingWidget->pos().x() + 6, m_ratingWidget->pos().y() + m_ratingWidget->boundingRect().height() ); // way bottom left corner
    statsPath.quadTo( m_ratingWidget->pos().x(), m_ratingWidget->pos().y() + m_ratingWidget->boundingRect().height(),
                    m_ratingWidget->pos().x(), m_ratingWidget->pos().y() + m_ratingWidget->boundingRect().height() - 6 );
    statsPath.lineTo( m_ratingWidget->pos().x(), m_ratingWidget->pos().y() + 6 ); // top left of rating widget
    statsPath.quadTo( m_ratingWidget->pos().x(), m_ratingWidget->pos().y(),
                    m_ratingWidget->pos().x() + 6, m_ratingWidget->pos().y() );
    statsPath.lineTo( leftEdge - 6, m_ratingWidget->pos().y() ); // joining of two rects
    statsPath.quadTo( leftEdge, m_ratingWidget->pos().y(),
                    leftEdge, m_ratingWidget->pos().y() - 6 );
    statsPath.lineTo( leftEdge, m_ratingWidget->pos().y() - m_ratingWidget->boundingRect().height() + 8 + 6 ); // back to start
    statsPath.quadTo( leftEdge, m_ratingWidget->pos().y() - m_ratingWidget->boundingRect().height() + 8,
                    leftEdge + 6, m_ratingWidget->pos().y() - m_ratingWidget->boundingRect().height() + 8 );

    p->fillPath( statsPath, bottomColor );
    // draw just the overlay which is the "header" row, to emphasize that we have 2 rows here
    QPainterPath headerPath;
    headerPath.moveTo( leftEdge + 6, m_ratingWidget->pos().y() - m_ratingWidget->boundingRect().height() + 8 ); // top left position of the rect, right below the album
    headerPath.lineTo( leftEdge + localMaxTextWidth - 6, m_ratingWidget->pos().y() - m_ratingWidget->boundingRect().height() + 8 ); // go right to margin
    headerPath.quadTo( leftEdge + localMaxTextWidth, m_ratingWidget->pos().y() - m_ratingWidget->boundingRect().height() + 8,
                    leftEdge + localMaxTextWidth, m_ratingWidget->pos().y() - m_ratingWidget->boundingRect().height() + 8 + 6 );
    headerPath.lineTo( leftEdge + localMaxTextWidth, m_ratingWidget->pos().y()  ); // middle of the right side
    headerPath.lineTo( leftEdge - 6, m_ratingWidget->pos().y() ); // join spot, before quad curve
    headerPath.quadTo( leftEdge, m_ratingWidget->pos().y(),
                        leftEdge, m_ratingWidget->pos().y() - 6 );
    headerPath.lineTo( leftEdge, m_ratingWidget->pos().y() - m_ratingWidget->boundingRect().height() + 8 + 6 ); // curve back through start
    headerPath.quadTo( leftEdge, m_ratingWidget->pos().y() - m_ratingWidget->boundingRect().height() + 8,
                       leftEdge + 6, m_ratingWidget->pos().y() - m_ratingWidget->boundingRect().height() + 8 );
                       
    p->fillPath( headerPath, topColor );
    p->restore();

    // draw label text
    p->save();
    // draw "Play count"
    QRectF rect = QRectF(leftEdge, // align vertically with track info text
                         m_ratingWidget->pos().y() - m_ratingWidget->boundingRect().height() + 8, // align bottom horizontally with top of rating rounded rect
                         localMaxTextWidth / 3,
                         m_ratingWidget->boundingRect().height() - 4 ); // just the "first" row, so go halfway down
    p->drawText( rect, Qt::AlignCenter | Qt::TextSingleLine, m_playCountLabel );
    rect.moveLeft( rect.topLeft().x() + localMaxTextWidth / 3 );
    p->drawText( rect, Qt::AlignCenter | Qt::TextSingleLine, m_scoreLabel );
    rect.moveLeft( rect.topLeft().x() + localMaxTextWidth / 3 );
    p->drawText( rect, Qt::AlignCenter | Qt::TextSingleLine, m_lastPlayedLabel );

    rect = QRectF( leftEdge,
                   m_ratingWidget->pos().y() + 3,
                   localMaxTextWidth / 3,
                   m_ratingWidget->boundingRect().height() - 4 );
    p->drawText( rect,  Qt::AlignCenter | Qt::TextSingleLine, m_numPlayed );
    rect.moveLeft( rect.topLeft().x() + localMaxTextWidth / 3 );
    p->drawText( rect, Qt::AlignCenter | Qt::TextSingleLine, m_score );
    rect.moveLeft( rect.topLeft().x() + localMaxTextWidth / 3 );
    p->drawText( rect, Qt::AlignCenter | Qt::TextSingleLine, m_playedLast );
    p->restore();
    
    // draw source emblem
    if( !m_sourceEmblemPixmap.isNull() )
    {
        p->save();
        p->drawPixmap(contentsRect.topRight().x() - m_sourceEmblemPixmap.rect().width() - m_margin,
                      m_margin,
                      m_sourceEmblemPixmap );
        p->restore();
    }
}

bool CurrentTrack::resizeCover( QPixmap cover, qreal width, QPointF albumCoverPos )
{
    const int borderWidth = 5;
    
    if( !cover.isNull() )
    {
        width -= borderWidth * 2;

        qreal size = width;
        qreal pixmapRatio = (qreal)cover.width()/size;

        // offset it by 20 so there is room for the ratings underneath to the left
        qreal moveByX = albumCoverPos.x();
        qreal moveByY = albumCoverPos.y();

        //center the cover : if the cover is not squared, we get the missing pixels and center
        if( cover.height()/pixmapRatio > width )
            cover = cover.scaledToHeight( size, Qt::SmoothTransformation );
        else
            cover = cover.scaledToWidth( size, Qt::SmoothTransformation );
        // center
        moveByX += ( width / 2 ) - cover.rect().width() / 2;
        moveByY += ( width / 2 ) - cover.rect().height() / 2;
        
        debug() << "placing album at X:" << moveByX << " and Y:"  << moveByY;
        m_albumCover->setPos( moveByX, moveByY );


        QPixmap coverWithBorders = The::svgHandler()->addBordersToPixmap( cover, borderWidth, m_album->text(), true );

        
        m_albumCover->setPixmap( coverWithBorders );
        return true;
    }
    return false;
}

void CurrentTrack::paletteChanged( const QPalette & palette )
{
    DEBUG_BLOCK

    m_title->setBrush( palette.text() );
    m_artist->setBrush( palette.text() );
    m_album->setBrush( palette.text() );
    m_noTrack->setBrush( palette.text() );
}

void
CurrentTrack::tabChanged( int index )
{
    Meta::TrackList tracks;
    if( index == 0 )
        tracks = m_lastTracks;
    else
        tracks = m_favoriteTracks;
    
    int i = 0;    
    foreach( Meta::TrackPtr track, tracks )
    {
        m_tracks[i]->hide();
        m_tracks[i]->setTrack( track );
        i++;
    }
    for( i = 0; i < m_tracksToShow; i++ )
        m_tracks[i]->show();
}

void
CurrentTrack::alignBottomToFirst( QGraphicsItem* a, QGraphicsItem* b )
{
    qreal guideY = a->pos().y() + a->boundingRect().height();
    qreal newY = guideY - b->boundingRect().height() + 1; // just a bit off for some reason (when used w/ text)
    b->setPos( b->x(), newY );
}

#include "CurrentTrack.moc"


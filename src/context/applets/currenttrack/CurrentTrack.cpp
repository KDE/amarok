/****************************************************************************************
 * Copyright (c) 2007-2009 Leo Franchi <lfranchi@gmail.com>                             *
 * Copyright (c) 2008 William Viana Soares <vianasw@gmail.com>                          *
 * Copyright (c) 2009 simon.esneault <simon.esneault@gmail.com>                         *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "CurrentTrack"

#include "CurrentTrack.h"

#include "core/support/Amarok.h"
#include "App.h"
#include "core/support/Debug.h"
#include "EngineController.h"
#include "GlobalCurrentTrackActions.h"
#include "CollectionManager.h"
#include "core/capabilities/EditCapability.h"
#include "core/capabilities/CurrentTrackActionsCapability.h"
#include "core/capabilities/FindInSourceCapability.h"
#include "core/meta/support/MetaUtility.h"
#include "PaletteHandler.h"
#include "SvgHandler.h"
#include "context/widgets/RatingWidget.h"
#include "context/widgets/TextScrollingWidget.h"
#include "context/widgets/DropPixmapItem.h"
#include "context/widgets/RecentlyPlayedListWidget.h"
#include "core/capabilities/UpdateCapability.h"
#include "dialogs/TagDialog.h"

#include <KConfigDialog>
#include <KGlobalSettings>
#include <KIconEffect>
#include <Plasma/IconWidget>
#include <Plasma/Label>

#include <QFont>
#include <QGraphicsAnchorLayout>
#include <QGraphicsLinearLayout>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>
#include <QPixmapCache>
#include <QScopedPointer>
#include <QSignalMapper>

CurrentTrack::CurrentTrack( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_actionsLayout( 0 )
    , m_findInSourceSignalMapper( 0 )
    , m_rating( -1 )
    , m_score( 0 )
    , m_trackLength( 0 )
    , m_playCount( 0 )
    , m_trackCount( 0 )
    , m_albumCount( 0 )
    , m_genreCount( 0 )
    , m_isStopped( true )
    , m_albumWidth( 135 )
{
    setHasConfigurationInterface( true );
    setBackgroundHints( Plasma::Applet::NoBackground );
}

CurrentTrack::~CurrentTrack()
{
}

void
CurrentTrack::init()
{
    DEBUG_BLOCK

    // Call the base implementation.
    Context::Applet::init();

    m_ratingWidget = new RatingWidget( this );
    m_ratingWidget->setSpacing( 2 );
    m_ratingWidget->setMinimumSize( m_albumWidth + 10, 30 );
    m_ratingWidget->setMaximumSize( m_albumWidth + 10, 30 );
    m_ratingWidget->hide();
    connect( m_ratingWidget, SIGNAL( ratingChanged( int ) ), SLOT( trackRatingChanged( int ) ) );

    m_collectionLabel = new Plasma::Label( this );
    m_collectionLabel->setAlignment( Qt::AlignCenter );
    m_collectionLabel->setText( i18n( "Local Collection" ) );
    m_collectionLabel->show();

    m_title  = new TextScrollingWidget( this );
    m_artist = new TextScrollingWidget( this );
    m_album  = new TextScrollingWidget( this );
    m_byText = new QGraphicsSimpleTextItem( i18nc( "What artist is this track by", "By" ), this );
    m_onText = new QGraphicsSimpleTextItem( i18nc( "What album is this track on", "On" ), this );

    m_recentWidget = new RecentlyPlayedListWidget( this );
    m_recentHeader = new TextScrollingWidget( this );
    m_recentHeader->setDrawBackground( true );
    m_recentHeader->setAlignment( Qt::AlignLeft );
    QFont recentHeaderFont;
    recentHeaderFont.setPointSize( recentHeaderFont.pointSize() + 2 );
    m_recentHeader->setFont( recentHeaderFont );

    m_title->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    m_artist->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    m_album->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    m_ratingWidget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    m_collectionLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    m_recentHeader->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    m_recentWidget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    m_recentWidget->setMinimumHeight( 10 );

    m_albumCover = new DropPixmapLayoutItem( this );
    m_albumCover->setPreferredSize( QSizeF( m_albumWidth, m_albumWidth ) );
    connect( m_albumCover, SIGNAL(imageDropped(QPixmap)), SLOT(coverDropped(QPixmap)) );

    const QBrush brush = normalBrush();
    m_title->setBrush( brush );
    m_artist->setBrush( brush );
    m_album->setBrush( brush );
    m_byText->setBrush( brush );
    m_onText->setBrush( brush );

    const QFont tinyFont = KGlobalSettings::smallestReadableFont();
    m_byText->setFont( tinyFont );
    m_onText->setFont( tinyFont );

    m_byText->hide();
    m_onText->hide();
    m_title->hide();
    m_artist->hide();
    m_album->hide();

    m_actionsLayout = new QGraphicsLinearLayout;
    m_actionsLayout->setMinimumWidth( 10 );
    m_actionsLayout->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    QGraphicsLinearLayout *titlesLayout = new QGraphicsLinearLayout( Qt::Vertical );
    titlesLayout->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    titlesLayout->setMinimumWidth( 10 );
    titlesLayout->setSpacing( 2 );
    titlesLayout->addItem( m_title );
    titlesLayout->addItem( m_artist );
    titlesLayout->addItem( m_album );
    titlesLayout->addItem( m_actionsLayout );
    titlesLayout->setItemSpacing( 2, 4 ); // a bit more spacing for the actions row

    const qreal pad = standardPadding();
    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout( this );
    l->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    l->addCornerAnchors( m_ratingWidget, Qt::BottomLeftCorner, l, Qt::BottomLeftCorner );
    l->addCornerAnchors( m_ratingWidget, Qt::BottomLeftCorner, m_collectionLabel, Qt::BottomLeftCorner );
    l->addCornerAnchors( m_ratingWidget, Qt::TopRightCorner, m_collectionLabel, Qt::TopRightCorner );
    l->addCornerAnchors( m_recentHeader, Qt::TopRightCorner, l, Qt::TopRightCorner );
    l->addAnchor( m_albumCover, Qt::AnchorBottom, m_ratingWidget, Qt::AnchorTop )->setSpacing( 4 );
    l->addAnchor( m_albumCover, Qt::AnchorHorizontalCenter, m_ratingWidget, Qt::AnchorHorizontalCenter );
    l->addAnchor( titlesLayout, Qt::AnchorTop, l, Qt::AnchorTop )->setSpacing( 18 );
    l->addAnchor( titlesLayout, Qt::AnchorRight, l, Qt::AnchorRight )->setSpacing( pad );
    l->addAnchor( m_recentHeader, Qt::AnchorLeft, titlesLayout, Qt::AnchorLeft );
    l->addAnchor( m_recentWidget, Qt::AnchorTop, m_recentHeader, Qt::AnchorBottom );
    l->addAnchor( m_recentWidget, Qt::AnchorRight, m_recentHeader, Qt::AnchorRight );
    l->addAnchor( m_recentWidget, Qt::AnchorLeft, m_ratingWidget, Qt::AnchorRight )->setSpacing( pad * 2 );
    l->addAnchor( m_recentWidget, Qt::AnchorBottom, m_albumCover, Qt::AnchorBottom )->setSpacing( 20 );
    qreal midSpacing = qMax( m_byText->boundingRect().width(), m_onText->boundingRect().width() ) + pad;
    l->addAnchor( titlesLayout, Qt::AnchorLeft, m_ratingWidget, Qt::AnchorRight )->setSpacing( midSpacing );
    l->anchor( m_recentHeader, Qt::AnchorTop, l, Qt::AnchorTop )->setSpacing( 2 );

    // Read config
    KConfigGroup config = Amarok::config("Current Track Applet");
    const QString fontDesc = config.readEntry( "Font", QString() );
    QFont font;
    if( !fontDesc.isEmpty() )
        font.fromString( fontDesc );
    else
        font.setPointSize( font.pointSize() + 3 );

    m_title->setFont( font );
    m_artist->setFont( font );
    m_album->setFont( font );

    m_title->setAlignment( Qt::AlignLeft );
    m_artist->setAlignment( Qt::AlignLeft );
    m_album->setAlignment( Qt::AlignLeft );

    dataEngine( "amarok-current" )->setProperty( "coverWidth", m_albumWidth );
    dataEngine( "amarok-current" )->connectSource( "current", this );
    connect( The::paletteHandler(), SIGNAL(newPalette(QPalette)), SLOT(paletteChanged(QPalette)) );

    // figure out the size we want to be, in order to be able to squeeze in all that we want
    // depends on the current font size,  basically
    // height should be increased for larger point sizes. here, the layout works correctly with size 8, which has the fontMetrics height of 13
    // a size too big, like font size 12, has a fontMetrics height of 19. So we add some height if it's too big
    int additional = ( QApplication::fontMetrics().height()-13 ) * 2;
    resize( 500, 180 + additional );

    connect( CollectionManager::instance(), SIGNAL(collectionDataChanged(Collections::Collection*)),
             this, SLOT(queryCollection()), Qt::QueuedConnection );
    queryCollection();
}

void
CurrentTrack::trackRatingChanged( int rating )
{
    Meta::TrackPtr track = The::engineController()->currentTrack();
    if( !track )
        return;

    // Inform collections of end of a metadata update
    QScopedPointer<Capabilities::UpdateCapability> uc( track->create<Capabilities::UpdateCapability>() );
    if( uc )
        track->setRating( rating );
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
    if( !album )
        return actions;

    QScopedPointer<Capabilities::CustomActionsCapability> cac( album->create<Capabilities::CustomActionsCapability>() );
    if( cac )
    {
        QList<QAction *> customActions = cac->customActions();
        foreach( QAction *action, customActions )
            actions.append( action );
    }
    return actions;
}

void
CurrentTrack::constraintsEvent( Plasma::Constraints constraints )
{
    Context::Applet::constraintsEvent( constraints );

    prepareGeometryChange();
    m_byText->setPos( m_artist->pos() );
    m_onText->setPos( m_album->pos() );
    m_byText->setX( m_byText->x() - m_byText->boundingRect().width() - 4 );
    m_onText->setX( m_onText->x() - m_onText->boundingRect().width() - 4 );
    alignBaseLineToFirst( m_artist, m_byText );
    alignBaseLineToFirst( m_album, m_onText );

    if( m_isStopped )
    {
        m_recentHeader->setScrollingText( i18n("Recently Played Tracks") );
        return;
    }

    QString artist = handleUnknown( m_artist->text(), m_artist, UNKNOWN_ARTIST.toString() );
    QString album = handleUnknown( m_album->text(), m_album, UNKNOWN_ALBUM.toString() );
    m_title->setScrollingText( m_title->text() );
    m_artist->setScrollingText( artist );
    m_album->setScrollingText( album );
}

void
CurrentTrack::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    if( data.isEmpty() || name != QLatin1String("current") )
        return;

    DEBUG_BLOCK
    clearTrackActions();
    if( data.contains( QLatin1String("notrack" ) ) )
    {
        m_isStopped = true;
        m_collectionLabel->show();
        m_ratingWidget->hide();
        m_byText->hide();
        m_onText->hide();
        m_title->hide();
        m_artist->hide();
        m_album->hide();
        m_recentWidget->show();
        m_recentHeader->show();
        m_recentWidget->startQuery();
        m_albumCover->setPixmap( amarokLogo(m_albumWidth) );
        m_albumCover->graphicsItem()->setAcceptDrops( false );
        updateConstraints();
        update();
        return;
    }

    m_isStopped = false;
    m_collectionLabel->hide();
    m_ratingWidget->show();
    m_byText->show();
    m_onText->show();
    m_title->show();
    m_artist->show();
    m_album->show();
    m_recentWidget->hide();
    m_recentHeader->hide();

    const QVariantMap &currentInfo = data[ QLatin1String("current") ].toMap();
    QString title = currentInfo.value( Meta::Field::TITLE ).toString();
    QString artist = currentInfo.value( Meta::Field::ARTIST ).toString();
    QString album = currentInfo.value( Meta::Field::ALBUM ).toString();
    artist = handleUnknown( artist, m_artist, UNKNOWN_ARTIST.toString() );
    album = handleUnknown( album, m_album, UNKNOWN_ALBUM.toString() );

    m_title->setScrollingText( title );
    m_artist->setScrollingText( artist );
    m_album->setScrollingText( album );

    m_rating      = currentInfo[ Meta::Field::RATING ].toInt();
    m_trackLength = currentInfo[ Meta::Field::LENGTH ].toInt();
    m_score       = currentInfo[ Meta::Field::SCORE ].toInt();
    m_lastPlayed  = currentInfo[ Meta::Field::LAST_PLAYED ].toDateTime();
    m_playCount   = currentInfo[ Meta::Field::PLAYCOUNT ].toInt();

    m_ratingWidget->setRating( m_rating );

    resizeCover( data[ "albumart" ].value<QPixmap>(), m_albumWidth );
    m_sourceEmblemPath = data[ "source_emblem" ].toString();

    setupLayoutActions( The::engineController()->currentTrack() );

    // without that the rating doesn't get update for a playing track
    updateConstraints();
    update();
}

void
CurrentTrack::editTrack()
{
    Meta::TrackPtr track = The::engineController()->currentTrack();
    new TagDialog( track, scene()->views().first() );
}

void
CurrentTrack::paintInterface( QPainter *p,
                              const QStyleOptionGraphicsItem *option,
                              const QRect &contentsRect )
{
    Context::Applet::paintInterface( p, option, contentsRect );
    addGradientToAppletBackground( p );
    drawStatsBackground( p );
    drawStatsTexts( p );
    drawSourceEmblem( p );
}

void
CurrentTrack::drawStatsBackground( QPainter *const p )
{
    // draw the complete outline. lots of little steps :) at each corner, leave
    // a 6x6 box. draw a quad bezier curve from the two ends of the lines,
    // through  the original corner

    const qreal leftEdge = m_ratingWidget->boundingRect().right() + standardPadding();
    const qreal rightEdge = boundingRect().size().width() - standardPadding();
    const qreal ratingWidgetX = m_ratingWidget->pos().x();
    const qreal ratingWidgetY = m_ratingWidget->pos().y();
    const qreal ratingWidgetH = m_ratingWidget->boundingRect().height();
    QColor bottomColor( 255, 255, 255, 90 );
    QColor topColor( 255, 255, 255, 120 );

    QPainterPath statsPath;
    statsPath.moveTo( leftEdge + 6, ratingWidgetY - ratingWidgetH + 8 ); // top left position of the rect, right below the album
    statsPath.lineTo( rightEdge - 6, ratingWidgetY - ratingWidgetH + 8 ); // go right to margin
    statsPath.quadTo( rightEdge, ratingWidgetY - ratingWidgetH + 8,
                      rightEdge, ratingWidgetY - ratingWidgetH + 8 + 6 );
    statsPath.lineTo( rightEdge, ratingWidgetY + ratingWidgetH - 6 ); // go down to bottom right corner
    statsPath.quadTo( rightEdge, ratingWidgetY + ratingWidgetH,
                      rightEdge - 6, ratingWidgetY + ratingWidgetH );
    statsPath.lineTo( ratingWidgetX + 6, ratingWidgetY + ratingWidgetH ); // way bottom left corner
    statsPath.quadTo( ratingWidgetX, ratingWidgetY + ratingWidgetH,
                      ratingWidgetX, ratingWidgetY + ratingWidgetH - 6 );
    statsPath.lineTo( ratingWidgetX, ratingWidgetY + 6 ); // top left of rating widget
    statsPath.quadTo( ratingWidgetX, ratingWidgetY,
                      ratingWidgetX + 6, ratingWidgetY );
    statsPath.lineTo( leftEdge - 6, ratingWidgetY ); // joining of two rects
    statsPath.quadTo( leftEdge, ratingWidgetY,
                      leftEdge, ratingWidgetY - 6 );
    statsPath.lineTo( leftEdge, ratingWidgetY - ratingWidgetH + 8 + 6 ); // back to start
    statsPath.quadTo( leftEdge, ratingWidgetY - ratingWidgetH + 8,
                      leftEdge + 6, ratingWidgetY - ratingWidgetH + 8 );

    // draw just the overlay which is the "header" row, to emphasize that we have 2 rows here
    QPainterPath headerPath;
    headerPath.moveTo( leftEdge + 6, ratingWidgetY - ratingWidgetH + 8 ); // top left position of the rect, right below the album
    headerPath.lineTo( rightEdge - 6, ratingWidgetY - ratingWidgetH + 8 ); // go right to margin
    headerPath.quadTo( rightEdge, ratingWidgetY - ratingWidgetH + 8,
                       rightEdge, ratingWidgetY - ratingWidgetH + 8 + 6 );
    headerPath.lineTo( rightEdge, ratingWidgetY  ); // middle of the right side
    headerPath.lineTo( leftEdge - 6, ratingWidgetY ); // join spot, before quad curve
    headerPath.quadTo( leftEdge, ratingWidgetY,
                       leftEdge, ratingWidgetY - 6 );
    headerPath.lineTo( leftEdge, ratingWidgetY - ratingWidgetH + 8 + 6 ); // curve back through start
    headerPath.quadTo( leftEdge, ratingWidgetY - ratingWidgetH + 8,
                       leftEdge + 6, ratingWidgetY - ratingWidgetH + 8 );

    p->save();
    p->fillPath( statsPath, bottomColor );
    p->fillPath( headerPath, topColor );
    p->restore();

}

void
CurrentTrack::drawStatsTexts( QPainter *const p )
{
    const qreal leftEdge       = m_ratingWidget->boundingRect().right() + standardPadding();
    const qreal maxTextWidth   = size().width() - standardPadding() * 2 - leftEdge;
    const QString column1Label = m_isStopped ? i18n( "Tracks" ) : i18n( "Play count" );
    const QString column2Label = m_isStopped ? i18n( "Albums" ) : i18n( "Score" );
    const QString column3Label = m_isStopped ? i18n( "Genres" ) : i18n( "Last Played" );

    //Align labels taking into account the string widths for each label
    QFontMetricsF fm( font() );
    qreal totalWidth = fm.width( column1Label ) + fm.width( column2Label ) + fm.width( column3Label );
    qreal factor, prevFactor;
    factor = fm.width( column1Label ) / totalWidth;
    prevFactor = factor;

    QRectF rect( leftEdge, // align vertically with track info text
                 m_ratingWidget->pos().y() - m_ratingWidget->boundingRect().height() + 8, // align bottom horizontally with top of rating rounded rect
                 maxTextWidth * factor,
                 m_ratingWidget->boundingRect().height() - 4 ); // just the "first" row, so go halfway down

    p->save();

    // labels
    QString playCountLabel = fm.elidedText( column1Label, Qt::ElideRight, rect.width() );
    p->drawText( rect, Qt::AlignCenter | Qt::TextSingleLine, playCountLabel );

    factor = fm.width( column2Label ) / totalWidth;
    rect.setWidth( maxTextWidth * factor );
    rect.moveLeft( rect.topLeft().x() + maxTextWidth * prevFactor );
    prevFactor = factor;

    QString scoreLabel = fm.elidedText( column2Label, Qt::ElideRight, rect.width() );
    p->drawText( rect, Qt::AlignCenter | Qt::TextSingleLine, scoreLabel );

    factor = fm.width( column3Label ) / totalWidth;
    rect.setWidth( maxTextWidth * factor );
    rect.moveLeft( rect.topLeft().x() + maxTextWidth * prevFactor );

    QString lastPlayedLabel = fm.elidedText( column3Label, Qt::ElideRight, rect.width() );
    p->drawText( rect, Qt::AlignCenter | Qt::TextSingleLine, lastPlayedLabel );

    // stats

    const int column1Stat = m_isStopped ? m_trackCount : m_playCount;
    const int column2Stat = m_isStopped ? m_albumCount : m_score;

    factor = fm.width( column1Label ) / totalWidth;
    prevFactor = factor;
    rect.setX( leftEdge );
    rect.setY( m_ratingWidget->pos().y() + 3 );
    rect.setWidth( maxTextWidth * factor);
    rect.setHeight( m_ratingWidget->boundingRect().height() - 4 );
    p->drawText( rect,  Qt::AlignCenter | Qt::TextSingleLine, QString::number(column1Stat) );

    factor = fm.width( column2Label ) / totalWidth;
    rect.setWidth( maxTextWidth * factor );
    rect.moveLeft( rect.topLeft().x() + maxTextWidth * prevFactor );
    prevFactor = factor;

    p->drawText( rect, Qt::AlignCenter | Qt::TextSingleLine, QString::number(column2Stat) );

    factor = fm.width( column3Label ) / totalWidth;
    rect.setWidth( maxTextWidth * factor );
    rect.moveLeft( rect.topLeft().x() + maxTextWidth * prevFactor );

    const QString column3Stat = ( m_isStopped )
        ? QString::number( m_genreCount )
        : fm.elidedText( Amarok::verboseTimeSince(m_lastPlayed), Qt::ElideRight, rect.width() );
    p->drawText( rect, Qt::AlignCenter | Qt::TextSingleLine, column3Stat );

    p->restore();
}

void
CurrentTrack::drawSourceEmblem( QPainter *const p )
{
    if( m_sourceEmblemPath.isEmpty() )
        return;

    p->save();
    p->setOpacity( 0.19 );
    QSvgRenderer svg( m_sourceEmblemPath );

    // paint the emblem half as tall as the applet, anchored at the top-right
    // assume it is a square emblem
    qreal height = boundingRect().height() / 2;
    QRectF rect( boundingRect().width() - standardPadding() - height, standardPadding(),
                 height, height );
    svg.render( p, rect );
    p->restore();
}

void
CurrentTrack::clearTrackActions()
{
    int actionCount = m_actionsLayout->count();
    while( --actionCount >= 0 )
    {
        QGraphicsLayoutItem *child = m_actionsLayout->itemAt( 0 );
        m_actionsLayout->removeItem( child );
        delete child;
    }
    qDeleteAll( m_customActions );
    m_customActions.clear();
}

void
CurrentTrack::resizeCover( const QPixmap &cover, qreal width )
{
    QPixmap coverWithBorders;
    if( !cover.isNull() )
    {
        const int borderWidth = 5;
        width -= borderWidth * 2;
        qreal pixmapRatio = (qreal)cover.width() / width;

        //center the cover : if the cover is not squared, we get the missing pixels and center
        coverWithBorders = ( cover.height() / pixmapRatio > width )
            ? cover.scaledToHeight( width, Qt::SmoothTransformation )
            : cover.scaledToWidth( width, Qt::SmoothTransformation );

        coverWithBorders = The::svgHandler()->addBordersToPixmap( coverWithBorders,
                                                                  borderWidth,
                                                                  m_album->text(),
                                                                  true );
    }
    m_albumCover->setPixmap( coverWithBorders );
    m_albumCover->graphicsItem()->setAcceptDrops( true );
}

QPixmap
CurrentTrack::amarokLogo( int dim ) const
{
    QPixmap logo;
    #define AMAROK_LOGO_CACHE_KEY QLatin1String("AmarokGraySemiTransparentIcon")
    if( !QPixmapCache::find( AMAROK_LOGO_CACHE_KEY, &logo ) )
    {
        QImage amarokIcon = KIcon( QLatin1String("amarok") ).pixmap( dim, dim ).toImage();
        KIconEffect::toGray( amarokIcon, 1 );
        KIconEffect::semiTransparent( amarokIcon );
        logo = QPixmap::fromImage( amarokIcon );
        QPixmapCache::insert( AMAROK_LOGO_CACHE_KEY, logo );
    }
    #undef AMAROK_LOGO_CACHE_KEY
    return logo;
}

void
CurrentTrack::fontChanged()
{
    QFont font = ui_Settings.fontChooser->font();

    m_title->setFont( font );
    m_artist->setFont( font );
    m_album->setFont( font );

    KConfigGroup config = Amarok::config("Current Track Applet");
    config.writeEntry( "Font", font.toString() );

    updateConstraints();
    update();
}

void
CurrentTrack::queryCollection()
{
    Collections::QueryMaker *qmTracks = CollectionManager::instance()->queryMaker();
    Collections::QueryMaker *qmAlbums = CollectionManager::instance()->queryMaker();
    Collections::QueryMaker *qmGenres = CollectionManager::instance()->queryMaker();
    connect( qmTracks, SIGNAL(newResultReady(QString, QStringList)),
             this, SLOT(tracksCounted(QString, QStringList)) );
    connect( qmAlbums, SIGNAL(newResultReady(QString, QStringList)),
             this, SLOT(albumsCounted(QString, QStringList)) );
    connect( qmGenres, SIGNAL(newResultReady(QString, QStringList)),
             this, SLOT(genresCounted(QString, QStringList)) );

    qmTracks->setAutoDelete( true )
      ->setQueryType( Collections::QueryMaker::Custom )
      ->addReturnFunction( Collections::QueryMaker::Count, Meta::valUrl )
      ->run();
    /* TODO: These don't work since not implemented in SqlQueryMaker::linkedTables()
    qmAlbums->setAutoDelete( true )
      ->setQueryType( Collections::QueryMaker::Custom )
      ->addReturnFunction( Collections::QueryMaker::Count, Meta::valAlbum )
      ->run();
    qmGenres->setAutoDelete( true )
      ->setQueryType( Collections::QueryMaker::Custom )
      ->addReturnFunction( Collections::QueryMaker::Count, Meta::valGenre )
      ->run();
      */
}

void
CurrentTrack::tracksCounted( QString id, QStringList results )
{
    Q_UNUSED( id );
    m_trackCount = !results.isEmpty() ? results.first().toInt() : 0;
    update();
}

void
CurrentTrack::albumsCounted( QString id, QStringList results )
{
    Q_UNUSED( id );
    m_albumCount = !results.isEmpty() ? results.first().toInt() : 0;
    update();
}

void
CurrentTrack::genresCounted( QString id, QStringList results )
{
    Q_UNUSED( id );
    m_genreCount = !results.isEmpty() ? results.first().toInt() : 0;
    update();
}

void
CurrentTrack::createConfigurationInterface( KConfigDialog *parent )
{
    KConfigGroup configuration = config();
    QWidget *settings = new QWidget;
    ui_Settings.setupUi( settings );
    ui_Settings.fontChooser->setFont( m_title->font() );

    parent->addPage( settings, i18n( "Current Track Settings" ), "preferences-system");

    connect( parent, SIGNAL( accepted() ), this, SLOT( fontChanged() ) );
}

void
CurrentTrack::coverDropped( const QPixmap &cover )
{
    DEBUG_BLOCK
    Meta::TrackPtr track = The::engineController()->currentTrack();
    if( !track )
        return;

    Meta::AlbumPtr album = track->album();
    if( !album )
        return;

    if ( !cover.isNull() )
        album->setImage( cover.toImage() );
}

void
CurrentTrack::paletteChanged( const QPalette & palette )
{
    DEBUG_BLOCK

    m_title->setBrush( palette.text() );
    m_artist->setBrush( palette.text() );
    m_album->setBrush( palette.text() );
    m_byText->setBrush( palette.text() );
    m_onText->setBrush( palette.text() );
}

void
CurrentTrack::alignBaseLineToFirst( TextScrollingWidget *a, QGraphicsSimpleTextItem *b )
{
    qreal guideY = a->pos().y() + QFontMetricsF( a->font() ).ascent();
    qreal newY   = guideY - QFontMetricsF( b->font() ).ascent();
    b->setPos( b->x(), newY );
}

QBrush
CurrentTrack::normalBrush() const
{
    return The::paletteHandler()->palette().brush( QPalette::Active, QPalette::Text );
}

QBrush
CurrentTrack::unknownBrush() const
{
    return The::paletteHandler()->palette().brush( QPalette::Disabled, QPalette::Text );
}

QString
CurrentTrack::handleUnknown( const QString &original,
                             TextScrollingWidget *widget,
                             const QString &replacement )
{
    if( original.isEmpty() )
    {
        widget->setBrush( unknownBrush() );
        return replacement;
    }
    else
    {
        widget->setBrush( normalBrush() );
        return original;
    }
}

void
CurrentTrack::setupLayoutActions( Meta::TrackPtr track )
{
    if( !track )
        return;

    //first, add any global CurrentTrackActions (iow, actions that are shown for all tracks)
    QList<QAction *> actions = The::globalCurrentTrackActions()->actions();

    using namespace Capabilities;

    if( track->hasCapabilityInterface( Capability::CurrentTrackActions ) )
    {
        QScopedPointer<CurrentTrackActionsCapability> cac( track->create<CurrentTrackActionsCapability>() );
        if( cac )
            m_customActions << cac->customActions();
    }

    if( track->hasCapabilityInterface( Capability::Editable ) )
    {
        QScopedPointer<EditCapability> ec( track->create<EditCapability>() );
        if( ec && ec->isEditable() )
        {
            QAction *editAction = new QAction( KIcon("media-track-edit-amarok"),
                                               i18n("Edit Track Details"), this );
            connect( editAction, SIGNAL(triggered()), SLOT(editTrack()) );
            m_customActions << editAction;
        }
    }

    if( track->hasCapabilityInterface( Capability::FindInSource ) )
    {
        if( !m_findInSourceSignalMapper )
        {
            m_findInSourceSignalMapper = new QSignalMapper( this );
            connect( m_findInSourceSignalMapper, SIGNAL(mapped(QString)), SLOT(findInSource(QString)) );
        }

        Meta::AlbumPtr album       = track->album();
        Meta::ArtistPtr artist     = track->artist();
        Meta::ComposerPtr composer = track->composer();
        Meta::GenrePtr genre       = track->genre();
        Meta::YearPtr year         = track->year();
        QAction *act( 0 );

        if( album && !album->name().isEmpty() )
        {
            act = new QAction( KIcon("current-track-amarok"), i18n("Show Album In Media Sources"), this );
            connect( act, SIGNAL(triggered()), m_findInSourceSignalMapper, SLOT(map()) );
            m_findInSourceSignalMapper->setMapping( act, QLatin1String("album") );
            m_customActions << act;
        }
        if( artist && !artist->name().isEmpty() )
        {
            act = new QAction( KIcon("filename-artist-amarok"), i18n("Show Artist In Media Sources"), this );
            connect( act, SIGNAL(triggered()), m_findInSourceSignalMapper, SLOT(map()) );
            m_findInSourceSignalMapper->setMapping( act, QLatin1String("artist") );
            m_customActions << act;
        }
        if( composer && !composer->name().isEmpty() && (composer->name() != i18n("Unknown Composer")) )
        {
            act = new QAction( KIcon("filename-composer-amarok"), i18n("Show Composer In Media Sources"), this );
            connect( act, SIGNAL(triggered()), m_findInSourceSignalMapper, SLOT(map()) );
            m_findInSourceSignalMapper->setMapping( act, QLatin1String("composer") );
            m_customActions << act;
        }
        if( genre && !genre->name().isEmpty() )
        {
            act = new QAction( KIcon("filename-genre-amarok"), i18n("Show Genre In Media Sources"), this );
            connect( act, SIGNAL(triggered()), m_findInSourceSignalMapper, SLOT(map()) );
            m_findInSourceSignalMapper->setMapping( act, QLatin1String("genre") );
            m_customActions << act;
        }
        if( year && !year->name().isEmpty() )
        {
            act = new QAction( KIcon("filename-year-amarok"), i18n("Show Year In Media Sources"), this );
            connect( act, SIGNAL(triggered()), m_findInSourceSignalMapper, SLOT(map()) );
            m_findInSourceSignalMapper->setMapping( act, QLatin1String("year") );
            m_customActions << act;
        }
    }

    actions << m_customActions;
    foreach( QAction* action, actions )
    {
        Plasma::IconWidget *icon = addAction( action, 24 );
        icon->setText( QString() );
        m_actionsLayout->addItem( icon );
    }
}

void
CurrentTrack::findInSource( const QString &name )
{
    using namespace Capabilities;
    Meta::TrackPtr track = The::engineController()->currentTrack();
    QScopedPointer<FindInSourceCapability> fis( track->create<FindInSourceCapability>() );
    if( !fis )
        return;

    if( name == QLatin1String("album") )
        fis->findInSource( FindInSourceCapability::Album );
    else if( name == QLatin1String("artist") )
        fis->findInSource( FindInSourceCapability::Artist );
    else if( name == QLatin1String("composer") )
        fis->findInSource( FindInSourceCapability::Composer );
    else if( name == QLatin1String("genre") )
        fis->findInSource( FindInSourceCapability::Genre );
    else if( name == QLatin1String("year") )
        fis->findInSource( FindInSourceCapability::Year );
}

#include "CurrentTrack.moc"

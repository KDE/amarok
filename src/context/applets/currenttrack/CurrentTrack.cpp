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

#include "App.h"
#include "EngineController.h"
#include "GlobalCurrentTrackActions.h"
#include "MainWindow.h"
#include "PaletteHandler.h"
#include "PluginManager.h"
#include "SvgHandler.h"
#include "amarokurls/AmarokUrl.h"
#include "context/widgets/RatingWidget.h"
#include "context/widgets/TextScrollingWidget.h"
#include "context/widgets/DropPixmapItem.h"
#include "context/widgets/RecentlyPlayedListWidget.h"
#include "core/capabilities/ActionsCapability.h"
#include "core/capabilities/BookmarkThisCapability.h"
#include "core/capabilities/FindInSourceCapability.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core/meta/Meta.h"
#include "core/meta/Statistics.h"
#include "core/meta/TrackEditor.h"
#include "core/meta/support/MetaUtility.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "covermanager/CoverViewDialog.h"
#include "dialogs/TagDialog.h"

#include <KConfigDialog>
#include <KGlobalSettings>
#include <Plasma/IconWidget>
#include <Plasma/Label>

#include <QFont>
#include <QGraphicsAnchorLayout>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>
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
    , m_artistCount( 0 )
    , m_isStopped( true )
    , m_coverKey( 0 )
    , m_view( Stopped )
    , m_showEditTrackDetailsAction( true )
    , m_albumWidth( 135 )
{
    setHasConfigurationInterface( true );
    setBackgroundHints( Plasma::Applet::NoBackground );
}

CurrentTrack::~CurrentTrack()
{
    clearTrackActions();
    delete m_albumCover;
}

void
CurrentTrack::init()
{
    DEBUG_BLOCK
    PERF_LOG( "Begin init" );

    // Call the base implementation.
    Context::Applet::init();

    setToolTip( i18n( "Right-click to configure" ) );

    m_ratingWidget = new RatingWidget( this );
    m_ratingWidget->setSpacing( 2 );
    m_ratingWidget->setMinimumSize( m_albumWidth + 10, 30 );
    m_ratingWidget->setMaximumSize( m_albumWidth + 10, 30 );
    connect( m_ratingWidget, SIGNAL(ratingChanged(int)), SLOT(trackRatingChanged(int)) );

    QLabel *collectionLabel = new QLabel( i18n( "Local Collection" ) );
    collectionLabel->setAttribute( Qt::WA_NoSystemBackground );
    collectionLabel->setAlignment( Qt::AlignCenter );
    m_collectionLabel = new QGraphicsProxyWidget( this );
    m_collectionLabel->setWidget( collectionLabel );

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
    l->addAnchors( m_recentWidget, m_recentHeader, Qt::Horizontal );
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

    m_showEditTrackDetailsAction = config.readEntry( "ShowEditTrackAction", true );

    m_title->setFont( font );
    m_artist->setFont( font );
    m_album->setFont( font );

    m_title->setAlignment( Qt::AlignLeft );
    m_artist->setAlignment( Qt::AlignLeft );
    m_album->setAlignment( Qt::AlignLeft );

    dataEngine( "amarok-current" )->setProperty( "coverWidth", m_albumWidth );
    dataEngine( "amarok-current" )->connectSource( "current", this );
    connect( The::paletteHandler(), SIGNAL(newPalette(QPalette)), SLOT(paletteChanged(QPalette)) );
    connect( CollectionManager::instance(), SIGNAL(collectionDataChanged(Collections::Collection*)),
             this, SLOT(queryCollection()), Qt::QueuedConnection );
    queryCollection();
    setView( Stopped );

    PERF_LOG( "Finished init" );
}

void
CurrentTrack::trackRatingChanged( int rating )
{
    Meta::TrackPtr track = The::engineController()->currentTrack();
    if( !track )
        return;

    track->statistics()->setRating( rating );
}

QList<QAction*>
CurrentTrack::contextualActions()
{
    DEBUG_BLOCK
    QList<QAction*> actions;
    Meta::TrackPtr track = The::engineController()->currentTrack();
    if( !track )
        return actions;

    if( !m_contextActions.isEmpty() )
        return m_contextActions;

    Meta::AlbumPtr album = track->album();
    if( !album )
        return actions;

    QScopedPointer<Capabilities::ActionsCapability> ac( album->create<Capabilities::ActionsCapability>() );
    if( ac )
    {
        m_contextActions << ac->actions();
        actions.append( m_contextActions );
    }
    return actions;
}

void
CurrentTrack::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
    if( !m_isStopped
        && event->modifiers() == Qt::NoModifier
        && event->button() == Qt::LeftButton )
    {
        QGraphicsView *view = scene()->views().first();
        QGraphicsItem *item = view->itemAt( view->mapFromScene(event->scenePos()) );
        if( item == m_albumCover->graphicsItem() )
        {
            Meta::AlbumPtr album = The::engineController()->currentTrack()->album();
            if( album )
                ( new CoverViewDialog( album, The::mainWindow() ) )->show();
            return;
        }
    }
    Context::Applet::mousePressEvent( event );
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

    update(); // ensure the stats bg is repainted with correct geometry
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

QSizeF
CurrentTrack::sizeHint( Qt::SizeHint which, const QSizeF &constraint ) const
{
    // figure out the size we want to be, in order to be able to squeeze in all
    // that we want depends on the current font size,  basically height should
    // be increased for larger point sizes. here, the layout works correctly
    // with size 8, which has the fontMetrics height of 13 a size too big, like
    // font size 12, has a fontMetrics height of 19. So we add some height if
    // it's too big
    int height = ( QApplication::fontMetrics().height() - 13 ) * 2 + 180;
    return QSizeF( Context::Applet::sizeHint(which, constraint).width(), height );
}

void
CurrentTrack::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    if( data.isEmpty() || name != QLatin1String("current") )
        return;

    if( data.contains( QLatin1String("notrack" ) ) )
    {
        if( m_view != Stopped )
            setView( Stopped );
        return;
    }

    const QPixmap &cover = data[ "albumart" ].value<QPixmap>();
    const QVariantMap &currentInfo = data[ QLatin1String("current") ].toMap();
    bool updateCover = ( m_coverKey != cover.cacheKey() );
    if( (m_currentInfo == currentInfo) && !updateCover )
        return;

    if( m_view != Playing )
        setView( Playing );

    QString title = currentInfo.value( Meta::Field::TITLE ).toString();
    QString artist = currentInfo.value( Meta::Field::ARTIST ).toString();
    QString album = currentInfo.value( Meta::Field::ALBUM ).toString();
    artist = handleUnknown( artist, m_artist, UNKNOWN_ARTIST.toString() );
    album = handleUnknown( album, m_album, UNKNOWN_ALBUM.toString() );

    if( title != m_currentInfo.value(Meta::Field::TITLE) )
        m_title->setScrollingText( title );
    if( artist != m_currentInfo.value(Meta::Field::ARTIST) )
        m_artist->setScrollingText( artist );
    if( album != m_currentInfo.value(Meta::Field::ALBUM) )
        m_album->setScrollingText( album );

    m_rating      = currentInfo[ Meta::Field::RATING ].toInt();
    m_trackLength = currentInfo[ Meta::Field::LENGTH ].toInt();
    m_score       = currentInfo[ Meta::Field::SCORE ].toInt();
    m_lastPlayed  = currentInfo[ Meta::Field::LAST_PLAYED ].toDateTime();
    m_playCount   = currentInfo[ Meta::Field::PLAYCOUNT ].toInt();

    m_ratingWidget->setRating( m_rating );

    if( updateCover )
    {
        m_coverKey = cover.cacheKey();
        resizeCover( cover, m_albumWidth );
    }
    m_currentInfo = currentInfo;
    m_sourceEmblemPath = data[ "source_emblem" ].toString();
    clearTrackActions();
    setupLayoutActions( The::engineController()->currentTrack() );
    updateConstraints();
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
    drawSourceEmblem( p, contentsRect );
    drawStatsBackground( p, contentsRect );
    drawStatsTexts( p, contentsRect );
}

void
CurrentTrack::drawStatsBackground( QPainter *const p, const QRect &rect )
{
    // draw the complete outline. lots of little steps :) at each corner, leave
    // a 6x6 box. draw a quad bezier curve from the two ends of the lines,
    // through  the original corner

    const qreal leftEdge = m_ratingWidget->boundingRect().right() + standardPadding();
    const qreal rightEdge = rect.right() - standardPadding() / 2;
    const qreal ratingWidgetX = m_ratingWidget->pos().x();
    const qreal ratingWidgetY = m_ratingWidget->pos().y();
    const qreal ratingWidgetH = m_ratingWidget->boundingRect().height();
    QColor topColor = The::paletteHandler()->palette().color( QPalette::Base );
    QColor bottomColor = topColor;
    topColor.setAlpha( 200 );
    bottomColor.setAlpha( 100 );

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
    p->setRenderHint( QPainter::Antialiasing );
    p->fillPath( statsPath, bottomColor );
    p->fillPath( headerPath, topColor );
    p->restore();

}

void
CurrentTrack::drawStatsTexts( QPainter *const p, const QRect &contentsRect )
{
    const qreal leftEdge       = m_ratingWidget->boundingRect().right() + standardPadding();
    const qreal maxTextWidth   = contentsRect.right() - standardPadding() * 2 - leftEdge;
    const QString column1Label = m_isStopped ? i18n( "Tracks" ) : i18n( "Play Count" );
    const QString column2Label = m_isStopped ? i18n( "Albums" ) : i18n( "Score" );
    const QString column3Label = m_isStopped ? i18n( "Artists" ) : i18n( "Last Played" );

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
    p->setRenderHint( QPainter::Antialiasing );
    p->setPen( normalBrush().color() );

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
        ? QString::number( m_artistCount )
        : fm.elidedText( Amarok::verboseTimeSince(m_lastPlayed), Qt::ElideRight, rect.width() );
    p->drawText( rect, Qt::AlignCenter | Qt::TextSingleLine, column3Stat );

    p->restore();
}

void
CurrentTrack::drawSourceEmblem( QPainter *const p, const QRect &contentsRect )
{
    if( m_isStopped )
        return;

    p->save();
    p->setOpacity( 0.19 );

    if( m_sourceEmblemPath.isEmpty() )
    {
        QPixmap logo = Amarok::semiTransparentLogo( m_albumWidth );
        QRect rect = logo.rect();
        int y = standardPadding();
        int x = contentsRect.right() - rect.width() - y;
        rect.moveTo( x, y );
        QRectF clipRect( rect );
        qreal clipY = m_ratingWidget->pos().y() - m_ratingWidget->boundingRect().height() + 8;
        clipRect.setBottom( clipY );
        p->setClipRect( clipRect );
        p->drawPixmap( rect, logo );
    }
    else
    {
        QSvgRenderer svg( m_sourceEmblemPath );
        // paint the emblem half as tall as the applet, anchored at the top-right
        // assume it is a square emblem
        qreal height = boundingRect().height() / 2;
        int y = standardPadding();
        int x = contentsRect.right() - y - height;
        QRectF rect( x, y, height, height );
        svg.render( p, rect );
    }
    p->restore();
}

void
CurrentTrack::clearTrackActions()
{
    prepareGeometryChange();
    int actionCount = m_actionsLayout->count();
    while( --actionCount >= 0 )
    {
        QGraphicsLayoutItem *child = m_actionsLayout->itemAt( 0 );
        m_actionsLayout->removeItem( child );
        delete child;
    }
    qDeleteAll( m_customActions );
    qDeleteAll( m_contextActions );
    m_customActions.clear();
    m_contextActions.clear();
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

void
CurrentTrack::settingsAccepted()
{
    QFont font = ui_Settings.fontRequester->font();
    m_showEditTrackDetailsAction = (ui_Settings.editTrackDetailsCheckBox->checkState() == Qt::Checked);

    m_title->setFont( font );
    m_artist->setFont( font );
    m_album->setFont( font );

    KConfigGroup config = Amarok::config("Current Track Applet");
    config.writeEntry( "Font", font.toString() );
    config.writeEntry( "ShowEditTrackAction", m_showEditTrackDetailsAction );

    clearTrackActions();
    setupLayoutActions( The::engineController()->currentTrack() );
}

void
CurrentTrack::queryCollection()
{
    Collections::QueryMaker *qmTracks = CollectionManager::instance()->queryMaker();
    Collections::QueryMaker *qmAlbums = CollectionManager::instance()->queryMaker();
    Collections::QueryMaker *qmArtists = CollectionManager::instance()->queryMaker();
    connect( qmTracks, SIGNAL(newResultReady(QStringList)),
             this, SLOT(tracksCounted(QStringList)) );
    connect( qmAlbums, SIGNAL(newResultReady(QStringList)),
             this, SLOT(albumsCounted(QStringList)) );
    connect( qmArtists, SIGNAL(newResultReady(QStringList)),
             this, SLOT(artistsCounted(QStringList)) );

    qmTracks->setAutoDelete( true )
      ->setQueryType( Collections::QueryMaker::Custom )
      ->addReturnFunction( Collections::QueryMaker::Count, Meta::valUrl )
      ->run();
    qmAlbums->setAutoDelete( true )
      ->setQueryType( Collections::QueryMaker::Custom )
      ->addReturnFunction( Collections::QueryMaker::Count, Meta::valAlbum )
      ->run();
    qmArtists->setAutoDelete( true )
      ->setQueryType( Collections::QueryMaker::Custom )
      ->addReturnFunction( Collections::QueryMaker::Count, Meta::valArtist )
      ->run();
}

void
CurrentTrack::tracksCounted( QStringList results )
{
    m_trackCount = !results.isEmpty() ? results.first().toInt() : 0;
    update();
}

void
CurrentTrack::albumsCounted( QStringList results )
{
    m_albumCount = !results.isEmpty() ? results.first().toInt() : 0;
    update();
}

void
CurrentTrack::artistsCounted( QStringList results )
{
    m_artistCount = !results.isEmpty() ? results.first().toInt() : 0;
    update();
}

void
CurrentTrack::createConfigurationInterface( KConfigDialog *parent )
{
    parent->setButtons( KDialog::Ok | KDialog::Cancel );

    KConfigGroup configuration = config();
    QWidget *settings = new QWidget;
    ui_Settings.setupUi( settings );
    ui_Settings.fontRequester->setFont( m_title->font() );
    ui_Settings.editTrackDetailsCheckBox->setCheckState( m_showEditTrackDetailsAction ? Qt::Checked : Qt::Unchecked );

    parent->addPage( settings, i18n( "Current Track Settings" ), "preferences-system");

    connect( parent, SIGNAL(accepted()), this, SLOT(settingsAccepted()) );
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
CurrentTrack::paletteChanged( const QPalette &palette )
{
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

    PERF_LOG( "Begin actions layout setup" );
    //first, add any global CurrentTrackActions (iow, actions that are shown for all tracks)
    QList<QAction *> actions = The::globalCurrentTrackActions()->actions();

    using namespace Capabilities;

    QScopedPointer<ActionsCapability> ac( track->create<ActionsCapability>() );
    if( ac )
    {
        QList<QAction*> trackActions = ac->actions();
        // ensure that the actions get deleted afterwards
        foreach( QAction* action, trackActions )
        {
            if( !action->parent() )
                action->setParent( this );
            actions << action;
        }
    }

    QScopedPointer<BookmarkThisCapability> btc( track->create<BookmarkThisCapability>() );
    if( btc && btc->bookmarkAction() )
    {
        actions << btc->bookmarkAction();
    }

    if( m_showEditTrackDetailsAction && track->editor() )
    {
        QAction *editAction = new QAction( KIcon("media-track-edit-amarok"),
                                           i18n("Edit Track Details"), this );
        connect( editAction, SIGNAL(triggered()), SLOT(editTrack()) );
        m_customActions << editAction;
    }

    if( track->has<FindInSourceCapability>() )
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
            act = new QAction( KIcon("current-track-amarok"), i18n("Show Album in Media Sources"), this );
            connect( act, SIGNAL(triggered()), m_findInSourceSignalMapper, SLOT(map()) );
            m_findInSourceSignalMapper->setMapping( act, QLatin1String("album") );
            m_customActions << act;
        }
        if( artist && !artist->name().isEmpty() )
        {
            act = new QAction( KIcon("filename-artist-amarok"), i18n("Show Artist in Media Sources"), this );
            connect( act, SIGNAL(triggered()), m_findInSourceSignalMapper, SLOT(map()) );
            m_findInSourceSignalMapper->setMapping( act, QLatin1String("artist") );
            m_customActions << act;

            // show a special action if the amazon plugin is enabled
            KPluginInfo::List services = The::pluginManager()->plugins( Plugins::PluginManager::Service );
            foreach( const KPluginInfo &service, services )
            {
                if( service.pluginName() == QLatin1String("amarok_service_amazonstore") )
                {
                    if( service.isPluginEnabled() )
                    {
                        act = new QAction( KIcon("view-services-amazon-amarok"),
                                           i18n("Search for Artist in the MP3 Music Store"), this );
                        connect( act, SIGNAL(triggered()), this, SLOT(findInStore()) );
                        m_customActions << act;
                    }
                    break;
                }
            }
        }
        if( composer && !composer->name().isEmpty() && (composer->name() != i18n("Unknown Composer")) )
        {
            act = new QAction( KIcon("filename-composer-amarok"), i18n("Show Composer in Media Sources"), this );
            connect( act, SIGNAL(triggered()), m_findInSourceSignalMapper, SLOT(map()) );
            m_findInSourceSignalMapper->setMapping( act, QLatin1String("composer") );
            m_customActions << act;
        }
        if( genre && !genre->name().isEmpty() )
        {
            act = new QAction( KIcon("filename-genre-amarok"), i18n("Show Genre in Media Sources"), this );
            connect( act, SIGNAL(triggered()), m_findInSourceSignalMapper, SLOT(map()) );
            m_findInSourceSignalMapper->setMapping( act, QLatin1String("genre") );
            m_customActions << act;
        }
        if( year && !year->name().isEmpty() )
        {
            act = new QAction( KIcon("filename-year-amarok"), i18n("Show Year in Media Sources"), this );
            connect( act, SIGNAL(triggered()), m_findInSourceSignalMapper, SLOT(map()) );
            m_findInSourceSignalMapper->setMapping( act, QLatin1String("year") );
            m_customActions << act;
        }
    }

    actions << m_customActions;
    foreach( QAction* action, actions )
    {
        Plasma::IconWidget *icon = addAction( this, action, 24 );
        icon->setText( QString() );
        m_actionsLayout->addItem( icon );
    }
    PERF_LOG( "Finished actions layout setup" );
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

void
CurrentTrack::findInStore()
{
    Meta::TrackPtr track = The::engineController()->currentTrack();
    AmarokUrl url( "amarok://navigate/internet/MP3%20Music%20Store/?filter=\"" + AmarokUrl::escape( track.data()->artist().data()->name() ) + '\"' );
    url.run();
}

void
CurrentTrack::setView( CurrentTrack::View mode )
{
    m_view = mode;
    m_isStopped = ( mode == CurrentTrack::Stopped );
    if( m_isStopped )
    {
        m_coverKey = 0;
        m_currentInfo.clear();
        m_sourceEmblemPath.clear();
        m_albumCover->setPixmap( Amarok::semiTransparentLogo(m_albumWidth) );
        m_albumCover->graphicsItem()->setAcceptDrops( false );
        m_albumCover->graphicsItem()->unsetCursor();
        clearTrackActions();
        updateConstraints();
    }
    else
    {
        m_albumCover->graphicsItem()->setCursor( Qt::PointingHandCursor );
    }

    m_collectionLabel->setVisible( m_isStopped );
    m_recentWidget->setVisible( m_isStopped );
    m_recentHeader->setVisible( m_isStopped );

    m_ratingWidget->setVisible( !m_isStopped );
    m_byText->setVisible( !m_isStopped );
    m_onText->setVisible( !m_isStopped );
    m_title->setVisible( !m_isStopped );
    m_artist->setVisible( !m_isStopped );
    m_album->setVisible( !m_isStopped );
}

#include "CurrentTrack.moc"

/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2008-2009 Nikolaj Hald Nielsen <nhn@kde.org>                           *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 * Copyright (c) 2010 Nanno Langstraat <langstr@gmail.com>                              *
 * Copyright (c) 2011 Sandeep Raghuraman <sandy.8925@gmail.com>                         *
 * Copyright (c) 2013 Mark Kretschmann <kretschmann@kde.org>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "Playlist::PrettyItemDelegate"

#include "PrettyItemDelegate.h"

#include "App.h"
#include "EngineController.h"
#include "MainWindow.h"
#include "PaletteHandler.h"
#include "SvgHandler.h"
#include "QStringx.h"
#include "core/support/Debug.h"
#include "core/meta/TrackEditor.h"
#include "core/capabilities/SourceInfoCapability.h"
#include "core/meta/Meta.h"
#include "core/meta/Statistics.h"
#include "moodbar/MoodbarManager.h"
#include "playlist/PlaylistModel.h"
#include "playlist/layouts/LayoutManager.h"
#include "playlist/proxymodels/GroupingProxy.h"
#include "playlist/view/listview/InlineEditorWidget.h"

#include <KColorScheme>
#include <KRatingPainter>  // #include <KratingPainter> does not work on some distros
#include <KWindowSystem>

#include <QAction>
#include <QFontMetricsF>
#include <QPainter>
#include <QStyleOptionSlider>
#include <QTimeLine>
#include <QTimer>

using namespace Playlist;

int Playlist::PrettyItemDelegate::s_fontHeight = 0;

Playlist::PrettyItemDelegate::PrettyItemDelegate( QObject* parent )
    : QStyledItemDelegate( parent )
{
    LayoutManager::instance();

    m_animationTimeLine = new QTimeLine( 900, this );
    m_animationTimeLine->setFrameRange( 1000, 600 );
    connect( m_animationTimeLine, &QTimeLine::frameChanged, this, &PrettyItemDelegate::redrawRequested );

#ifdef Q_WS_X11
    connect( KWindowSystem::self(), &KWindowSystem::currentDesktopChanged, this, &PrettyItemDelegate::currentDesktopChanged );
#endif
    connect( EngineController::instance(), &EngineController::playbackStateChanged, this, &PrettyItemDelegate::redrawRequested );
}

PrettyItemDelegate::~PrettyItemDelegate() { }

int PrettyItemDelegate::getGroupMode( const QModelIndex &index)
{
    return index.data( GroupRole ).toInt();
}

int
PrettyItemDelegate::rowsForItem( const QModelIndex &index )
{
    PlaylistLayout layout = LayoutManager::instance()->activeLayout();
    int rowCount = 0;

    if( getGroupMode( index ) == Grouping::Head )
        rowCount += layout.layoutForPart( PlaylistLayout::Head ).rows();

    rowCount += layout.layoutForItem( index ).rows();

    return rowCount;
}

QSize
PrettyItemDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    s_fontHeight = option.fontMetrics.height();

    // -- calculate the item height
    int rowCount = rowsForItem( index );

    if( LayoutManager::instance()->activeLayout().inlineControls() && index.data( ActiveTrackRole ).toBool() )
        rowCount++; //add room for extras

    QStyle *style;
    if( QWidget *w = qobject_cast<QWidget*>(parent()) )
        style = w->style();
    else
        style = QApplication::style();

    // note: we have to be as high as the InlineEditorWidget or that would
    // force re-layouts or overlap
    // on the other hand we squeeze the line edits quite a lot.
    int frameVMargin = style->pixelMetric( QStyle::PM_FocusFrameVMargin );
    int height = rowCount * s_fontHeight + ( rowCount + 1 ) * frameVMargin;
    return QSize( s_fontHeight * 20, height );
}

void
PrettyItemDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    PlaylistLayout layout = LayoutManager::instance()->activeLayout();

    painter->save();
    QApplication::style()->drawPrimitive( QStyle::PE_PanelItemViewItem, &option, painter );
    painter->translate( option.rect.topLeft() );

    painter->drawPixmap( 0, 0, The::svgHandler()->renderSvgWithDividers( QStringLiteral("track"), ( int )option.rect.width(), ( int )option.rect.height(), QStringLiteral("track") ) );

    painter->setPen( The::paletteHandler()->foregroundColor( painter, option.state & QStyle::State_Selected ) );

    // call paint method based on type
    const int groupMode = getGroupMode(index);

    int rowCount = rowsForItem( index );
    bool paintInlineControls = LayoutManager::instance()->activeLayout().inlineControls() && index.data( ActiveTrackRole ).toBool();

    if ( groupMode == Grouping::None ||  groupMode == Grouping::Body || groupMode == Grouping::Tail )
    {
        int trackHeight = 0;
        int extraHeight = 0;
        QStyleOptionViewItem trackOption( option );
        if ( paintInlineControls )
        {
            int adjustedRowCount = rowCount + 1;
            trackHeight = ( option.rect.height() * rowCount ) / adjustedRowCount + 3;
            extraHeight = option.rect.height() - trackHeight;
            trackOption.rect = QRect( 0, 0, option.rect.width(), trackHeight );
        }

        paintItem( layout.layoutForItem( index ), painter, trackOption, index );

        if (paintInlineControls )
        {
            QRect extrasRect( 0, trackHeight, option.rect.width(), extraHeight );
            paintActiveTrackExtras( extrasRect, painter, index );
        }
    }
    else if ( groupMode == Grouping::Head )
    {
        //we need to split up the options for the actual header and the included first track

        QFont boldfont( option.font );
        boldfont.setBold( true );

        QStyleOptionViewItem headOption( option );
        QStyleOptionViewItem trackOption( option );

        int headRows = layout.layoutForPart( PlaylistLayout::Head ).rows();
        int trackRows = layout.layoutForItem( index ).rows();
        int totalRows = headRows + trackRows;

        //if this layout is completely empty, bail out or we will get in divide-by-zero trouble
        if ( totalRows == 0 )
        {
            painter->restore();
            return;
        }

        if ( paintInlineControls )
        {
            totalRows = totalRows + 1;
        }

        int headHeight = ( headRows * option.rect.height() ) / totalRows - 2;
        int trackHeight = ( trackRows * option.rect.height() ) / totalRows + 2;

        if ( headRows > 0 )
        {
            headOption.rect = QRect( 0, 0, option.rect.width(), headHeight );
            paintItem( layout.layoutForPart( PlaylistLayout::Head ), painter, headOption, index, true );
            painter->translate( 0, headHeight );
        }

        trackOption.rect = QRect( 0, 0, option.rect.width(), trackHeight );
        paintItem( layout.layoutForItem( index ), painter, trackOption, index );

        if ( paintInlineControls )
        {
            int extraHeight = option.rect.height() - ( headHeight + trackHeight );
            QRect extrasRect( 0, trackHeight, option.rect.width(), extraHeight );
            paintActiveTrackExtras( extrasRect, painter, index );

        }
    }
    else
        QStyledItemDelegate::paint( painter, option, index );

    painter->restore();
}

bool
PrettyItemDelegate::insideItemHeader( const QPoint& pt, const QRect& rect )
{
    QRect headerBounds = rect;
    headerBounds.setHeight( headerHeight() );

    return headerBounds.contains( pt );
}

int
PrettyItemDelegate::headerHeight() const
{
    int headRows = LayoutManager::instance()->activeLayout().layoutForPart( PlaylistLayout::Head ).rows();

    if( headRows < 1 )
        return 0;

    QStyle *style;
    if( QWidget *w = qobject_cast<QWidget*>(parent()) )
        style = w->style();
    else
        style = QApplication::style();

    int frameVMargin = style->pixelMetric( QStyle::PM_FocusFrameVMargin );
    return headRows * ( s_fontHeight + frameVMargin );
}

QPointF
PrettyItemDelegate::centerImage( const QPixmap& pixmap, const QRectF& rect ) const
{
    qreal pixmapRatio = ( qreal )pixmap.width() / ( qreal )pixmap.height();

    qreal moveByX = 0.0;
    qreal moveByY = 0.0;

    if ( pixmapRatio >= 1 )
        moveByY = ( rect.height() - ( rect.width() / pixmapRatio ) ) / 2.0;
    else
        moveByX = ( rect.width() - ( rect.height() * pixmapRatio ) ) / 2.0;

    return QPointF( moveByX, moveByY );
}


void Playlist::PrettyItemDelegate::paintItem( const LayoutItemConfig &config,
                                              QPainter* painter,
                                              const QStyleOptionViewItem& option,
                                              const QModelIndex& index,
                                              bool headerRow ) const
{
    const int rowCount = config.rows();
    if( rowCount == 0 )
        return;

    QStyle *style;
    if( QWidget *w = qobject_cast<QWidget*>(parent()) )
        style = w->style();
    else
        style = QApplication::style();

    // we have to use the same metrics as the InlineEditorWidget or else
    // the widgets will change places when editing
    const int horizontalSpace = style->pixelMetric( QStyle::PM_LayoutHorizontalSpacing );
    const int smallIconSize = style->pixelMetric( QStyle::PM_SmallIconSize );
    const int frameHMargin = style->pixelMetric( QStyle::PM_FocusFrameHMargin );
    const int frameVMargin = style->pixelMetric( QStyle::PM_FocusFrameVMargin );
    const int iconSpacing = style->pixelMetric( QStyle::PM_ToolBarItemSpacing );

    int rowOffsetX = frameHMargin; // keep the text a little bit away from the border
    int rowOffsetY = frameVMargin;

    const int coverHeight = option.rect.height() - frameVMargin * 2;
    QRectF nominalImageRect( frameHMargin, frameVMargin, coverHeight, coverHeight );

    const bool showCover = config.showCover();
    if( showCover )
        rowOffsetX += coverHeight + horizontalSpace/* + frameHMargin * 2*/;

    const int contentHeight = option.rect.height() - frameVMargin * 2;
    int rowHeight = contentHeight / rowCount;
    const int rowWidth = option.rect.width() - rowOffsetX - frameHMargin * 2;

    // --- paint the active track background
    // We do not want to paint this for head items.
    if( !headerRow && index.data( ActiveTrackRole ).toBool() )
    {
        //paint this in 3 parts to solve stretching issues with wide playlists
        //TODO: proper 9 part painting, but I don't want to bother with this until we
        //get some new graphics anyway...

        // -- try not to highlight the indicator row
        int overlayXOffset = 0;
        int overlayYOffset = config.activeIndicatorRow() * rowHeight;
        int overlayHeight = option.rect.height() - overlayYOffset;
        int overlayLength = option.rect.width();

        int endWidth = overlayHeight / 4;

        if( m_animationTimeLine->currentFrame() == m_animationTimeLine->startFrame() && EngineController::instance()->isPlaying() ) {
            m_animationTimeLine->setDirection( QTimeLine::Forward );
            if( m_animationTimeLine->state() == QTimeLine::NotRunning )
                m_animationTimeLine->start();
        }
        else if( m_animationTimeLine->currentFrame() == m_animationTimeLine->endFrame() ) {
            m_animationTimeLine->setDirection( QTimeLine::Backward );
            m_animationTimeLine->start();
        }

        // Opacity is used for animating the active track item
        const qreal opacity = qreal( m_animationTimeLine->currentFrame() ) / 1000;

        // If opacity is not the default value we cannot render from cache
        const bool skipCache = opacity == 1.0 ? false : true;

        painter->drawPixmap( overlayXOffset, overlayYOffset,
                             The::svgHandler()->renderSvg( QStringLiteral("active_overlay_left"),
                                                           endWidth,
                                                           overlayHeight,
                                                           QStringLiteral("active_overlay_left"),
                                                           skipCache,
                                                           opacity ) );

        painter->drawPixmap( overlayXOffset + endWidth, overlayYOffset,
                             The::svgHandler()->renderSvg( QStringLiteral("active_overlay_mid"),
                                                           overlayLength - endWidth * 2,
                                                           overlayHeight,
                                                           QStringLiteral("active_overlay_mid"),
                                                           skipCache,
                                                           opacity ) );

        painter->drawPixmap( overlayXOffset + ( overlayLength - endWidth ), overlayYOffset,
                             The::svgHandler()->renderSvg( QStringLiteral("active_overlay_right"),
                                                           endWidth,
                                                           overlayHeight,
                                                           QStringLiteral("active_overlay_right"),
                                                           skipCache,
                                                           opacity ) );
    }

    // --- paint the cover
    if( showCover )
    {
        QModelIndex coverIndex = index.model()->index( index.row(), CoverImage );
        QPixmap albumPixmap = coverIndex.data( Qt::DisplayRole ).value<QPixmap>();

        if( !albumPixmap.isNull() )
        {
            //offset cover if non square
            QPointF offset = centerImage( albumPixmap, nominalImageRect );
            QRectF imageRect( nominalImageRect.x() + offset.x(),
                              nominalImageRect.y() + offset.y(),
                              nominalImageRect.width() - offset.x() * 2,
                              nominalImageRect.height() - offset.y() * 2 );

            painter->drawPixmap( imageRect, albumPixmap, QRectF( albumPixmap.rect() ) );
        }

        QModelIndex emblemIndex = index.model()->index( index.row(), SourceEmblem );
        QPixmap emblemPixmap = emblemIndex.data( Qt::DisplayRole ).value<QPixmap>();

        if( !emblemPixmap.isNull() )
            painter->drawPixmap( QRectF( nominalImageRect.x(), nominalImageRect.y() , 16, 16 ), emblemPixmap, QRectF( 0, 0 , 16, 16 ) );
    }

    // --- paint the markers
    int markerOffsetX = frameHMargin;
    const int rowOffsetXBeforeMarkers = rowOffsetX;
    if( !headerRow )
    {
        const int queuePosition = index.data( QueuePositionRole ).toInt();
        if( queuePosition > 0 )
        {
            const int x = markerOffsetX;
            const int y = nominalImageRect.y() + ( coverHeight - smallIconSize );
            const QString number = QString::number( queuePosition );
            const int iconWidth = option.fontMetrics.boundingRect( number ).width() + smallIconSize / 2;

            const QRect rect( x, y, iconWidth, smallIconSize ); // shift text by 1 pixel left

            const KColorScheme colorScheme( option.palette.currentColorGroup() );
            QPen rectanglePen = painter->pen();
            rectanglePen.setColor( colorScheme.foreground( KColorScheme::PositiveText ).color() );
            QBrush rectangleBrush = colorScheme.background( KColorScheme::PositiveBackground );

            painter->save();
            painter->setPen( rectanglePen );
            painter->setBrush( rectangleBrush );
            painter->setRenderHint( QPainter::Antialiasing, true );
            painter->drawRoundedRect( QRect( x, y - 1, iconWidth, smallIconSize ), smallIconSize / 3, smallIconSize / 3 );
            painter->drawText( rect, Qt::AlignCenter, number );
            painter->restore();

            markerOffsetX += ( iconWidth + iconSpacing );

            if ( !showCover )
                rowOffsetX += ( iconWidth + iconSpacing );
            else
                rowOffsetX += qMax( 0, iconWidth - smallIconSize - iconSpacing );
        }

        if( index.data( MultiSourceRole ).toBool() )
        {
            const int x = markerOffsetX;
            const int y = nominalImageRect.y() + ( coverHeight - smallIconSize );
            painter->drawPixmap( x, y, The::svgHandler()->renderSvg( QStringLiteral("multi_marker"), smallIconSize, smallIconSize, QStringLiteral("multi_marker") ) );

            markerOffsetX += ( smallIconSize + iconSpacing );

            if ( !showCover )
                rowOffsetX += ( smallIconSize + iconSpacing );
        }

        if( index.data( StopAfterTrackRole ).toBool() )
        {
            const int x = markerOffsetX;
            const int y = nominalImageRect.y() + ( coverHeight - smallIconSize );
            painter->drawPixmap( x, y, The::svgHandler()->renderSvg( QStringLiteral("stop_button"), smallIconSize, smallIconSize, QStringLiteral("stop_button") ) );

            markerOffsetX += ( smallIconSize + iconSpacing );

            if ( !showCover )
                rowOffsetX += ( smallIconSize + iconSpacing );
        }
    }
    int markersWidth = rowOffsetX - rowOffsetXBeforeMarkers;

    Meta::TrackPtr trackPtr = index.data( TrackRole ).value<Meta::TrackPtr>();
    QMap<QString, QString> trackArgs;
    if( trackPtr )
        trackArgs = buildTrackArgsMap( trackPtr );

    // --- paint all the rows
    for( int i = 0; i < rowCount; i++ )
    {
        LayoutItemConfigRow row = config.row( i );
        const int elementCount = row.count();

        //we need to do a quick pass to figure out how much space is left for auto sizing elements
        qreal spareSpace = 1.0;
        int autoSizeElemCount = 0;
        for( int k = 0; k < elementCount; ++k )
        {
            spareSpace -= row.element( k ).size();
            if( row.element( k ).size() < 0.001 )
                autoSizeElemCount++;
        }

        const qreal spacePerAutoSizeElem = spareSpace / (qreal)autoSizeElemCount;

        //give left over pixels to the first rows. Widgets are doing it the same.
        if( i == 0 )
            rowHeight++;
        if( i == ( contentHeight % rowCount ) )
            rowHeight--;

        int currentItemX = rowOffsetX;
        for( int j = 0; j < elementCount; ++j )
        {
            LayoutItemConfigRowElement element = row.element( j );

            // -- calculate the size
            qreal size;
            if( element.size() < 0.001 )
                size = spacePerAutoSizeElem;
            else
                size = element.size();

            qreal itemWidth;
            if( j == elementCount - 1 )
                // use the full with for the last item
                itemWidth = rowWidth - (currentItemX - rowOffsetXBeforeMarkers);
            else
                itemWidth = rowWidth * size - markersWidth;

            markersWidth = 0; // leave columns > 0 alone, they are unaffected by markers

            if( itemWidth <= 1 )
                continue; // no sense to paint such small items

            // -- set font
            bool bold = element.bold();
            bool italic = element.italic();
            bool underline = element.underline();
            int alignment = element.alignment();

            QFont font = option.font;
            font.setBold( bold );
            font.setItalic( italic );
            font.setUnderline( underline );
            painter->setFont( font );

            int value = element.value();
            QModelIndex textIndex = index.model()->index( index.row(), value );

            // -- paint the element
            if( value == Rating )
            {
                int rating = textIndex.data( Qt::DisplayRole ).toInt();

                KRatingPainter::paintRating( painter,
                                             QRect( currentItemX, rowOffsetY,
                                                    itemWidth, rowHeight ),
                                             element.alignment(), rating );

            }
            else if( value == Divider )
            {
                QPixmap left = The::svgHandler()->renderSvg( QStringLiteral("divider_left"),
                                                             1, rowHeight ,
                                                             QStringLiteral("divider_left") );

                QPixmap right = The::svgHandler()->renderSvg( QStringLiteral("divider_right"),
                                                              1, rowHeight,
                                                              QStringLiteral("divider_right") );

                if( alignment & Qt::AlignLeft )
                {
                    painter->drawPixmap( currentItemX, rowOffsetY, left );
                    painter->drawPixmap( currentItemX + 1, rowOffsetY, right );
                }
                else if( alignment & Qt::AlignRight )
                {
                    painter->drawPixmap( currentItemX + itemWidth - 1, rowOffsetY, left );
                    painter->drawPixmap( currentItemX + itemWidth, rowOffsetY, right );
                }
                else
                {
                    int center = currentItemX + ( itemWidth / 2 );
                    painter->drawPixmap( center, rowOffsetY, left );
                    painter->drawPixmap( center + 1, rowOffsetY, right );
                }
            }
            else if( value == Moodbar )
            {
                //we cannot ask the model for the moodbar directly as we have no
                //way of asking for a specific size. Instead just get the track from
                //the model and ask the moodbar manager ourselves.
                Meta::TrackPtr track = index.data( TrackRole ).value<Meta::TrackPtr>();

                if( The::moodbarManager()->hasMoodbar( track ) )
                {
                    QPixmap moodbar = The::moodbarManager()->getMoodbar( track, itemWidth, rowHeight - 8 );

                    painter->drawPixmap( currentItemX, rowOffsetY + 4, moodbar );
                }
            }
            //actual playlist item text is drawn here
            else
            {
                //TODO: get rid of passing TrackPtr as data, use custom role instead
                Meta::TrackPtr track = index.data( TrackRole ).value<Meta::TrackPtr>();
                QString text = textIndex.data( Qt::DisplayRole ).toString();
                Amarok::QStringx prefix( element.prefix() );
                Amarok::QStringx suffix( element.suffix() );
                text = prefix.namedOptArgs( trackArgs ) + text + suffix.namedOptArgs( trackArgs );
                text = QFontMetricsF( font ).elidedText( text, Qt::ElideRight, itemWidth );

                //if the track can't be played, it should be grayed out to show that it is unavailable
                if( !track->isPlayable() )
                {
                    painter->save();
                    QPen grayPen = painter->pen();
                    grayPen.setColor( QColor( 127, 127, 127 ) ); // TODO: use disabled role
                    painter->setPen( grayPen );
                    painter->drawText( QRect( currentItemX + frameHMargin, rowOffsetY,
                                              itemWidth - frameHMargin * 2, rowHeight ),
                                       alignment, text );
                    painter->restore();
                }
                else
                {
                    painter->drawText( QRect( currentItemX + frameHMargin, rowOffsetY,
                                              itemWidth - frameHMargin * 2, rowHeight ),
                                       alignment, text );
                }
            }
            currentItemX += itemWidth;
        }
        rowOffsetY += rowHeight;
    }
}

void Playlist::PrettyItemDelegate::paintActiveTrackExtras( const QRect &rect, QPainter* painter, const QModelIndex& index ) const
{
    Q_UNUSED( index );

    int x = rect.x();
    int y = rect.y();
    int width = rect.width();
    int height = rect.height();
    int buttonSize = height - 4;

    QStyle *style;
    if( QWidget *w = qobject_cast<QWidget*>(parent()) )
        style = w->style();
    else
        style = QApplication::style();

    // some style margins:
    int frameHMargin = style->pixelMetric( QStyle::PM_FocusFrameHMargin );
    int iconSpacing = style->pixelMetric( QStyle::PM_ToolBarItemSpacing );

    //just paint some "buttons for now

    int offset = x + frameHMargin;
    painter->drawPixmap( offset, y + 2, buttonSize, buttonSize,
                         The::svgHandler()->renderSvg( QStringLiteral("back_button"),
                                                       buttonSize * 2, buttonSize * 2,
                                                       QStringLiteral("back_button") ) );

    if( The::engineController()->isPlaying() )
    {
        offset += ( buttonSize + iconSpacing );
        painter->drawPixmap( offset, y + 2, buttonSize, buttonSize,
                             The::svgHandler()->renderSvg( QStringLiteral("pause_button"),
                                                       buttonSize * 2, buttonSize * 2,
                                                           QStringLiteral("pause_button") ) );

    }
    else
    {
        offset += ( buttonSize + iconSpacing );
        painter->drawPixmap( offset, y + 2, buttonSize, buttonSize,
                             The::svgHandler()->renderSvg( QStringLiteral("play_button"),
                                                       buttonSize * 2, buttonSize * 2,
                                                           QStringLiteral("play_button") ) );
    }

    offset += ( buttonSize + iconSpacing );
    painter->drawPixmap( offset, y + 2, buttonSize, buttonSize,
                         The::svgHandler()->renderSvg( QStringLiteral("stop_button"),
                                                       buttonSize * 2, buttonSize * 2,
                                                       QStringLiteral("stop_button") ) );

    offset += ( buttonSize + iconSpacing );
    painter->drawPixmap( offset, y + 2, buttonSize, buttonSize,
                         The::svgHandler()->renderSvg( QStringLiteral("next_button"),
                                                       buttonSize * 2, buttonSize * 2,
                                                       QStringLiteral("next_button") ) );

    offset += ( buttonSize + iconSpacing );

    long trackLength = The::engineController()->trackLength();
    long trackPos = The::engineController()->trackPositionMs();
    qreal trackPercentage = 0.0;

    QStyleOptionSlider opt;
    if ( trackLength > 0 )
    {
        opt.state = QStyle::State_Enabled;
        trackPercentage = ( (qreal) trackPos / (qreal) trackLength );
    }

    int sliderWidth = width - ( offset + frameHMargin );
    opt.rect.setRect( offset, y, sliderWidth, height );
    The::svgHandler()->paintCustomSlider( painter, &opt, trackPercentage, false );
}

bool Playlist::PrettyItemDelegate::clicked( const QPoint &pos, const QRect &itemRect, const QModelIndex& index )
{

    //for now, only handle clicks in the currently playing item.
    if ( !index.data( ActiveTrackRole ).toBool() )
        return false;

    //also, if we are not using the inline controls, we should not react to these clicks at all
    if( !LayoutManager::instance()->activeLayout().inlineControls() )
        return false;

    int rowCount = rowsForItem( index );
    int modifiedRowCount = rowCount + 1;

    int height = itemRect.height();

    int baseHeight = ( height * rowCount ) / modifiedRowCount + 3;
    int extrasHeight = height - baseHeight;
    int extrasOffsetY = height - extrasHeight;

    int buttonSize = extrasHeight - 4;

    QStyle *style;
    if( QWidget *w = qobject_cast<QWidget*>(parent()) )
        style = w->style();
    else
        style = QApplication::style();

    // some style margins:
    int frameHMargin = style->pixelMetric( QStyle::PM_FocusFrameHMargin );
    int iconSpacing = style->pixelMetric( QStyle::PM_ToolBarItemSpacing );

    int offset = frameHMargin;
    QRect backRect( offset, extrasOffsetY + 2, buttonSize, buttonSize );
    if( backRect.contains( pos ) )
    {
         Amarok::actionCollection()->action( QStringLiteral("prev") )->trigger();
         return true;
    }

    offset += ( buttonSize + iconSpacing );
    QRect playRect( offset, extrasOffsetY + 2, buttonSize, buttonSize );
    if( playRect.contains( pos ) )
    {
         Amarok::actionCollection()->action( QStringLiteral("play_pause") )->trigger();
         return true;
    }

    offset += ( buttonSize + iconSpacing );
    QRect stopRect( offset, extrasOffsetY + 2, buttonSize, buttonSize );
    if( stopRect.contains( pos ) )
    {
         Amarok::actionCollection()->action( QStringLiteral("stop") )->trigger();
         return true;
    }


    offset += ( buttonSize + iconSpacing );
    QRect nextRect( offset, extrasOffsetY + 2, buttonSize, buttonSize );
    if( nextRect.contains( pos ) )
    {
         Amarok::actionCollection()->action( QStringLiteral("next") )->trigger();
         return true;
    }

    offset += ( buttonSize + iconSpacing );

    //handle clicks on the slider

    int sliderWidth = itemRect.width() - ( offset + iconSpacing );
    int knobSize = buttonSize - 2;

    QRect sliderActiveRect( offset, extrasOffsetY + 3, sliderWidth, knobSize );
    if( sliderActiveRect.contains( pos ) )
    {
        int xSliderPos = pos.x() - offset;
        long trackLength = EngineController::instance()->trackLength();

        qreal percentage = (qreal) xSliderPos / (qreal) sliderWidth;
        EngineController::instance()->seekTo( trackLength * percentage );
        return true;

    }

    return false;
}

QWidget* Playlist::PrettyItemDelegate::createEditor( QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    Q_UNUSED( option );

    int editorHeight = sizeHint(option, index).height();
    int editorWidth = sizeHint(option, index).width();
    if( getGroupMode( index ) == Grouping::Head )
        editorHeight -= headerHeight();
    InlineEditorWidget *editor = new InlineEditorWidget( parent, index,
                     LayoutManager::instance()->activeLayout(), editorHeight, editorWidth );

    connect( editor, &InlineEditorWidget::editingDone,
             this, &PrettyItemDelegate::editorDone );
    return editor;
}

void Playlist::PrettyItemDelegate::setModelData( QWidget * editor, QAbstractItemModel * model, const QModelIndex &index ) const
{
    Q_UNUSED( model )

    InlineEditorWidget * inlineEditor = qobject_cast<InlineEditorWidget *>( editor );
    if( !inlineEditor )
        return;

    QMap<int, QString> changeMap = inlineEditor->changedValues();

    debug() << "got inline editor!!";
    debug() << "changed values map: " << changeMap;

    //ok, now get the track, figure out if it is editable and if so, apply new values.
    //It's as simple as that! :-)

    Meta::TrackPtr track = index.data( TrackRole ).value<Meta::TrackPtr>();
    if( !track )
        return;

    // this does not require TrackEditor
    if( changeMap.contains( Rating ) )
    {
        int rating = changeMap.value( Rating ).toInt();
        track->statistics()->setRating( rating );
        changeMap.remove( Rating );
    }

    Meta::TrackEditorPtr ec = track->editor();
    if( !ec )
        return;

    QList<int> columns = changeMap.keys();

    foreach( int column, columns )
    {
        QString value = changeMap.value( column );

        switch( column )
        {
            case Album:
                ec->setAlbum( value );
                break;
            case Artist:
                ec->setArtist( value );
                break;
            case Comment:
                ec->setComment( value );
                break;
            case Composer:
                ec->setComposer( value );
                break;
            case DiscNumber:
                {
                    int discNumber = value.toInt();
                    ec->setDiscNumber( discNumber );
                    break;
                }
            case Genre:
                ec->setGenre( value );
                break;
            case Rating:
                break; // we've already set the rating, this even shouldn't be here
            case Title:
                ec->setTitle( value );
                break;
            case TitleWithTrackNum:
                {
                    debug() << "parse TitleWithTrackNum";
                    //we need to parse out the track number and the track name (and check
                    //if the string is even valid...)
                    //QRegExp rx("(\\d+)\\s-\\s(.*))");
                    QRegExp rx("(\\d+)(\\s-\\s)(.*)");
                    if ( rx.indexIn( value ) != -1) {
                        int trackNumber = rx.cap( 1 ).toInt();
                        QString trackName = rx.cap( 3 );
                        debug() << "split TitleWithTrackNum into " << trackNumber << " and " << trackName;
                        ec->setTrackNumber( trackNumber );
                        ec->setTitle( trackName );
                    }
                    break;
                }
            case TrackNumber:
                {
                    int TrackNumber = value.toInt();
                    ec->setTrackNumber( TrackNumber );
                    break;
                }
            case Year:
                ec->setYear( value.toInt() );
                break;
            case Bpm:
                ec->setBpm( value.toFloat() );
                break;
        }
    }
}

void
Playlist::PrettyItemDelegate::updateEditorGeometry( QWidget * editor, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    Q_UNUSED( index )

    QRect editorRect( option.rect );
    if( getGroupMode( index ) == Grouping::Head )
        editorRect.setTop( editorRect.top() + headerHeight() );

    editor->setFixedSize( editorRect.size() );
    editor->setGeometry( editorRect );
}

void
Playlist::PrettyItemDelegate::editorDone( InlineEditorWidget * editor )
{
    Q_EMIT commitData( editor );
}

void
Playlist::PrettyItemDelegate::currentDesktopChanged()
{
    // Optimization for X11/Linux desktops:
    // Don't update the animation if Amarok is not on the active virtual desktop.

    m_animationTimeLine->setPaused( !The::mainWindow()->isOnCurrentDesktop() );
}

QMap<QString, QString>
Playlist::PrettyItemDelegate::buildTrackArgsMap( const Meta::TrackPtr &track ) const
{
    QMap<QString, QString> args;
    QString artist = track->artist() ? track->artist()->name() : QString();
    QString albumartist;
    if( track->album() && track->album()->hasAlbumArtist() )
        albumartist = track->album()->albumArtist()->name();
    else
        albumartist = artist;


    args[QStringLiteral("title")] = track->name();
    args[QStringLiteral("composer")] = track->composer() ? track->composer()->name() : QString();

    // if year == 0 then we don't want include it
    QString year = track->year() ? track->year()->name() : QString();
    args[QStringLiteral("year")] = year.localeAwareCompare( QStringLiteral("0") ) == 0 ? QString() : year;
    args[QStringLiteral("album")] = track->album() ? track->album()->name() : QString();

    if( track->discNumber() )
        args[QStringLiteral("discnumber")] = QString::number( track->discNumber() );

    args[QStringLiteral("genre")] = track->genre() ? track->genre()->name() : QString();
    args[QStringLiteral("comment")] = track->comment();
    args[QStringLiteral("artist")] = artist;
    args[QStringLiteral("albumartist")] = albumartist;
    args[QStringLiteral("initial")] = albumartist.mid( 0, 1 ).toUpper();    //artists starting with The are already handled above
    args[QStringLiteral("filetype")] = track->type();

    args[QStringLiteral("rating")] = track->statistics()->rating();
    args[QStringLiteral("filesize")] = track->filesize();
    args[QStringLiteral("length")] = track->length() / 1000;

    if ( track->trackNumber() )
    {
        QString trackNum = QStringLiteral( "%1" ).arg( track->trackNumber(), 2, 10, QChar('0') );
        args[QStringLiteral("track")] = trackNum;
    }

    return args;
}


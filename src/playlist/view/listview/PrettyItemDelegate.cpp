/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2008-2009 Nikolaj Hald Nielsen <nhn@kde.org>                           *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
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
#include "Debug.h"
#include "EngineController.h"
#include "InlineEditorWidget.h"
#include "SvgHandler.h"
#include "SvgTinter.h"
#include "meta/Meta.h"
#include "meta/capabilities/EditCapability.h"
#include "meta/capabilities/SourceInfoCapability.h"
#include "moodbar/MoodbarManager.h"
#include "playlist/proxymodels/GroupingProxy.h"
#include "playlist/PlaylistModel.h"
#include "playlist/layouts/LayoutManager.h"

#include "kratingpainter.h"

#include <QFontMetricsF>
#include <QPainter>
#include <QAction>
#include <QStyleOptionSlider>

using namespace Playlist;

int Playlist::PrettyItemDelegate::s_fontHeight = 0;


Playlist::PrettyItemDelegate::PrettyItemDelegate( QObject* parent )
    : QStyledItemDelegate( parent )
{
    DEBUG_BLOCK

    LayoutManager::instance();
}

PrettyItemDelegate::~PrettyItemDelegate() { }

int PrettyItemDelegate::getGroupMode( const QModelIndex &index) const
{
    return index.data( GroupRole ).toInt();
}

int PrettyItemDelegate::rowsForItem( const QModelIndex &index ) const
{

    PlaylistLayout layout = LayoutManager::instance()->activeLayout();

    const int groupMode = getGroupMode(index);
    int rowCount = 1;

    switch ( groupMode )
    {
        case Head:
            rowCount = layout.head().rows() + layout.body().rows();
            break;

        case Body:
            rowCount = layout.body().rows();
            break;

        case Tail:
            rowCount = layout.body().rows();
            break;

        case None:
        default:
            rowCount = layout.single().rows();
            break;
    }

    return rowCount;
}

QSize
PrettyItemDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    int height = 0;

    QFontMetricsF nfm( option.font );
    QFont boldfont( option.font );
    boldfont.setBold( true );
    QFontMetricsF bfm( boldfont );

    s_fontHeight = bfm.height();

    int rowCount = rowsForItem( index );

    if( LayoutManager::instance()->activeLayout().inlineControls() && index.data( ActiveTrackRole ).toBool() )
        rowCount++; //add room for extras

    height = MARGIN * 2 + rowCount * s_fontHeight + ( rowCount - 1 ) * PADDING;
    return QSize( 120, height );
}

void
PrettyItemDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    PlaylistLayout layout = LayoutManager::instance()->activeLayout();

    painter->save();
    QApplication::style()->drawPrimitive( QStyle::PE_PanelItemViewItem, &option, painter );
    painter->translate( option.rect.topLeft() );

    painter->drawPixmap( 0, 0, The::svgHandler()->renderSvgWithDividers( "track", ( int )option.rect.width(), ( int )option.rect.height(), "track" ) );

    if ( option.state & QStyle::State_Selected )
        painter->setPen( App::instance()->palette().highlightedText().color() );
    else
        painter->setPen( App::instance()->palette().text().color() );

    // call paint method based on type
    const int groupMode = getGroupMode(index);

    int rowCount = rowsForItem( index );
    bool paintInlineControls = LayoutManager::instance()->activeLayout().inlineControls() && index.data( ActiveTrackRole ).toBool();

    if ( groupMode == None ||  groupMode == Body || groupMode == Tail )
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

        if ( groupMode == None )
            paintItem( layout.single(), painter, trackOption, index );
        else if ( groupMode == Body )
            paintItem( layout.body(), painter, trackOption, index );
        else
            paintItem( layout.body(), painter, trackOption, index );

        if (paintInlineControls )
        {
            QRect extrasRect( 0, trackHeight, option.rect.width(), extraHeight );
            paintActiveTrackExtras( extrasRect, painter, index );

        }
    }
    else if ( groupMode == Head )
    {
        //we need to split up the options for the actual header and the included first track

        QFont boldfont( option.font );
        boldfont.setBold( true );
        QFontMetricsF bfm( boldfont );

        QStyleOptionViewItem headOption( option );
        QStyleOptionViewItem trackOption( option );

        int headRows = layout.head().rows();
        int trackRows = layout.body().rows();
        int totalRows = headRows + trackRows;

        //if this layout is completely empty, bail out or we will get in divide-by-zero trouble
        if ( totalRows == 0 )
            return;

        if ( paintInlineControls )
        {
            totalRows = totalRows + 1;
        }

        int headHeight = ( headRows * option.rect.height() ) / totalRows - 2;
        int trackHeight = ( trackRows * option.rect.height() ) / totalRows + 3;

        if ( headRows > 0 )
        {
            headOption.rect = QRect( 0, 0, option.rect.width(), headHeight );
            paintItem( layout.head(), painter, headOption, index, true );
            painter->translate( 0, headHeight );
        }

        trackOption.rect = QRect( 0, 0, option.rect.width(), trackHeight );
        paintItem( layout.body(), painter, trackOption, index );

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
    int headRows = LayoutManager::instance()->activeLayout().head().rows();

    if ( headRows < 1 )
        return false;

    QRect headerBounds = rect.adjusted( ( int )MARGINH,
                                        ( int )MARGIN,
                                        ( int )( -MARGINH ),
                                                    0 );

    headerBounds.setHeight( static_cast<int>( 2 * MARGIN + headRows * s_fontHeight ) );

    return headerBounds.contains( pt );
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


void Playlist::PrettyItemDelegate::paintItem( LayoutItemConfig config, QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index, bool ignoreMarkers ) const
{
    int rowCount = config.rows();

    if ( rowCount == 0 )
        return;

    int rowHeight = option.rect.height() / rowCount;

    int rowOffsetX = MARGINH;
    int rowOffsetY = 0;

    int imageSize = option.rect.height() - MARGIN * 2;
    QRectF nominalImageRect( MARGINH, MARGIN, imageSize, imageSize );

    int markerOffsetX = nominalImageRect.x();

    const bool showCover = config.showCover();

    if ( showCover )
        rowOffsetX = imageSize + MARGINH + PADDING * 2;

    for ( int i = 0; i < rowCount; i++ )
    {
        LayoutItemConfigRow row = config.row( i );
        qreal itemOffsetX = rowOffsetX;

        const int elementCount = row.count();

        qreal rowWidth = option.rect.width() - ( rowOffsetX + MARGINH );

        //We do not want to paint this for head items.
        if ( !ignoreMarkers && i == config.activeIndicatorRow() && index.data( ActiveTrackRole ).toBool() )
        {

            //paint this in 3 parts to solve stretching issues with wide playlists
            //TODO: propper 9 part painting, but I dont want to bother with this until we
            //get some new graphics anyway...

            int overlayXOffset = 0;
            int overlayYOffset = rowOffsetY - 1;
            int overlayHeight = option.rect.height() + 1;
            int overlayLength = option.rect.width();
            int endWidth = overlayHeight / 4;

            painter->drawPixmap( overlayXOffset, overlayYOffset,
                                 The::svgHandler()->renderSvg(
                                 "active_overlay_left",
                                 endWidth, overlayHeight,
                                 "active_overlay_left" ) );

            painter->drawPixmap( overlayXOffset + endWidth, overlayYOffset,
                                 The::svgHandler()->renderSvg(
                                 "active_overlay_mid",
                                 overlayLength - endWidth * 2, overlayHeight,
                                 "active_overlay_mid" ) );

            painter->drawPixmap( overlayXOffset + ( overlayLength - endWidth ), overlayYOffset,
                                 The::svgHandler()->renderSvg(
                                 "active_overlay_right",
                                 endWidth, overlayHeight,
                                 "active_overlay_right" ) );
        }

        QRectF rowBox( itemOffsetX, rowOffsetY, rowWidth, rowHeight );
        int currentItemX = itemOffsetX;

        //we need to do a quick pass to figure out how much space is left for auto sizing elements
        qreal spareSpace = 1.0;
        int autoSizeElemCount = 0;
        for ( int k = 0; k < elementCount; ++k )
        {
            spareSpace -= row.element( k ).size();
            if ( row.element( k ).size() < 0.001 )
                autoSizeElemCount++;
        }

        qreal spacePerAutoSizeElem = spareSpace / (qreal) autoSizeElemCount;

        for ( int j = 0; j < elementCount; ++j )
        {
            LayoutItemConfigRowElement element = row.element( j );

            int value = element.value();

            QModelIndex textIndex = index.model()->index( index.row(), value );
            QString text = textIndex.data( Qt::DisplayRole ).toString();

            qreal itemWidth = 0.0;

            text = element.prefix() + text + element.suffix();

            bool bold = element.bold();
            bool italic = element.italic();
            bool underline = element.underline();
            int alignment = element.alignment();

            QFont font = option.font;
            font.setBold( bold );
            font.setItalic( italic );
            font.setUnderline( underline );
            painter->setFont( font );

            QRectF elementBox;

            qreal size;
            if ( element.size() > 0.0001 )
                size = element.size();
            else
                size = spacePerAutoSizeElem;

            if ( size > 0.0001 )
            {
                itemWidth = rowWidth * size;

                //special case for painting the rating...
                if ( value == Rating )
                {
                    int rating = textIndex.data( Qt::DisplayRole ).toInt();

                    Qt::Alignment ratingAlignment;
                    if ( alignment & Qt::AlignLeft )
                        ratingAlignment = Qt::AlignLeft;
                    else if ( alignment & Qt::AlignRight )
                        ratingAlignment = Qt::AlignRight;
                    else
                        ratingAlignment = Qt::AlignCenter;

                    KRatingPainter::paintRating( painter, QRect( currentItemX, rowOffsetY + 1, itemWidth, rowHeight - 2 ), ratingAlignment, rating, rating );

                } else if ( value == Divider )
                {
                    QPixmap left = The::svgHandler()->renderSvg(
                            "divider_left",
                            1, rowHeight ,
                            "divider_left" );

                    QPixmap right = The::svgHandler()->renderSvg(
                            "divider_right",
                            1, rowHeight,
                            "divider_right" );

                    if ( alignment & Qt::AlignLeft )
                    {
                        painter->drawPixmap( currentItemX, rowOffsetY, left );
                        painter->drawPixmap( currentItemX + 1, rowOffsetY, right );
                    }
                    else if ( alignment & Qt::AlignRight )
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


                    debug() << "painting moodbar in PrettyItemDelegate::paintItem";

                    Meta::TrackPtr track = index.data( TrackRole ).value<Meta::TrackPtr>();

                    if( The::moodbarManager()->hasMoodbar( track ) )
                    {
                        QPixmap moodbar = The::moodbarManager()->getMoodbar( track, itemWidth, rowHeight - 8 );

                        painter->drawPixmap( currentItemX, rowOffsetY + 4, moodbar );
                    }
                }
                else
                {
                    text = QFontMetricsF( font ).elidedText( text, Qt::ElideRight, itemWidth );
                    painter->drawText( currentItemX, rowOffsetY, itemWidth, rowHeight, alignment, text );
                }

                currentItemX += itemWidth;
            }

        }
        rowOffsetY += rowHeight;
    }

    if ( showCover )
    {
        QModelIndex coverIndex = index.model()->index( index.row(), CoverImage );
        QPixmap albumPixmap = coverIndex.data( Qt::DisplayRole ).value<QPixmap>();

        //offset cover if non square
        QPointF offset = centerImage( albumPixmap, nominalImageRect );
        QRectF imageRect( nominalImageRect.x() + offset.x(),
                          nominalImageRect.y() + offset.y(),
                          nominalImageRect.width() - offset.x() * 2,
                          nominalImageRect.height() - offset.y() * 2 );

        if ( !albumPixmap.isNull() )
            painter->drawPixmap( imageRect, albumPixmap, QRectF( albumPixmap.rect() ) );

        QModelIndex emblemIndex = index.model()->index( index.row(), SourceEmblem );
        QPixmap emblemPixmap = emblemIndex.data( Qt::DisplayRole ).value<QPixmap>();

        if ( !albumPixmap.isNull() )
            painter->drawPixmap( QRectF( nominalImageRect.x(), nominalImageRect.y() , 16, 16 ), emblemPixmap, QRectF( 0, 0 , 16, 16 ) );
    }

    if( !ignoreMarkers && index.data( StateRole ).toInt() & Item::Queued )
    {
        // Check that the queue position is actually valid
        const int queuePosition = index.data( QueuePositionRole ).toInt();
        if( queuePosition > 0 )
        {
            const int w = 16, h = 16;
            const int x = markerOffsetX;
            const int y = nominalImageRect.y() + ( imageSize - h );
            const QRect rect( x, y, w, h );
            painter->drawPixmap( x, y, The::svgHandler()->renderSvg( "queue_marker", w, h, "queue_marker" ) );
            painter->drawText( rect, Qt::AlignCenter, QString::number( queuePosition ) );

            markerOffsetX += ( 16 + PADDING );

            if ( !showCover )
                rowOffsetX = markerOffsetX;
        }
        else
            warning() << "discrepancy: Item::Queued but queuePosition == 0";
    }

    if( !ignoreMarkers && index.data( MultiSourceRole ).toBool() )
    {
        const int w = 16, h = 16;
        const int x = markerOffsetX;
        const int y = nominalImageRect.y() + ( imageSize - h );
        const QRect rect( x, y, w, h );
        painter->drawPixmap( x, y, The::svgHandler()->renderSvg( "multi_marker", w, h, "multi_marker" ) );

        markerOffsetX += ( 16 + PADDING );

        if ( !showCover )
            rowOffsetX += ( 16 + PADDING );
    }

    if( !ignoreMarkers && index.data( StopAfterTrackRole ).toBool() )
    {
        const int w = 16, h = 16;
        const int x = markerOffsetX;
        const int y = nominalImageRect.y() + ( imageSize - h );
        const QRect rect( x, y, w, h );
        painter->drawPixmap( x, y, The::svgHandler()->renderSvg( "stop_button", w, h, "stop_button" ) );

        markerOffsetX += ( 16 + PADDING );

        if ( !showCover )
            rowOffsetX += ( 16 + PADDING );
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

    //just paint some "buttons for now

    int offset = x + MARGINH;
    painter->drawPixmap( offset, y + 2,
                         The::svgHandler()->renderSvg(
                         "back_button",
                         buttonSize, buttonSize,
                         "back_button" ) );

    if ( EngineController::instance()->state() == Phonon::PlayingState ||
         EngineController::instance()->state() == Phonon::PlayingState )
    {
        offset += ( buttonSize + MARGINH );
        painter->drawPixmap( offset, y + 2,
                            The::svgHandler()->renderSvg(
                            "pause_button",
                            buttonSize, buttonSize,
                            "pause_button" ) );

    }
    else
    {

    offset += ( buttonSize + MARGINH );
    painter->drawPixmap( offset, y + 2,
                            The::svgHandler()->renderSvg(
                            "play_button",
                            buttonSize, buttonSize,
                            "play_button" ) );
    }

    offset += ( buttonSize + MARGINH );
    painter->drawPixmap( offset, y + 2,
                         The::svgHandler()->renderSvg(
                         "stop_button",
                         buttonSize, buttonSize,
                         "stop_button" ) );

    offset += ( buttonSize + MARGINH );
    painter->drawPixmap( offset, y + 2,
                         The::svgHandler()->renderSvg(
                         "next_button",
                         buttonSize, buttonSize,
                         "next_button" ) );

    offset += ( buttonSize + MARGINH );

    long trackLength = EngineController::instance()->trackLength();
    long trackPos = EngineController::instance()->trackPositionMs();
    qreal trackPercentage = 0.0;

    if ( trackLength > 0 )
        trackPercentage = ( (qreal) trackPos / (qreal) trackLength );

    int sliderWidth = width - ( offset + MARGINH );
    QStyleOptionSlider opt;
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

    int offset = MARGINH;
    QRect backRect( offset, extrasOffsetY + 2, buttonSize, buttonSize );
    if( backRect.contains( pos ) )
    {
         Amarok::actionCollection()->action( "prev" )->trigger();
         return true;
    }

    offset += ( buttonSize + MARGINH );
    QRect playRect( offset, extrasOffsetY + 2, buttonSize, buttonSize );
    if( playRect.contains( pos ) )
    {
         Amarok::actionCollection()->action( "play_pause" )->trigger();
         return true;
    }

    offset += ( buttonSize + MARGINH );
    QRect stopRect( offset, extrasOffsetY + 2, buttonSize, buttonSize );
    if( stopRect.contains( pos ) )
    {
         Amarok::actionCollection()->action( "stop" )->trigger();
         return true;
    }


    offset += ( buttonSize + MARGINH );
    QRect nextRect( offset, extrasOffsetY + 2, buttonSize, buttonSize );
    if( nextRect.contains( pos ) )
    {
         Amarok::actionCollection()->action( "next" )->trigger();
         return true;
    }

    offset += ( buttonSize + MARGINH );

    //handle clicks on the slider

    int sliderWidth = itemRect.width() - ( offset + MARGINH );
    int knobSize = buttonSize - 2;

    QRect sliderActiveRect( offset, extrasOffsetY + 3, sliderWidth, knobSize );
    if( sliderActiveRect.contains( pos ) )
    {
        int xSliderPos = pos.x() - offset;
        long trackLength = EngineController::instance()->trackLength();

        qreal percentage = (qreal) xSliderPos / (qreal) sliderWidth;
        EngineController::instance()->seek( trackLength * percentage );
        return true;

    }


    return false;
}

QWidget * Playlist::PrettyItemDelegate::createEditor ( QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    Q_UNUSED( option );

    DEBUG_BLOCK
    const int groupMode = getGroupMode(index);
    InlineEditorWidget * editor = new InlineEditorWidget( parent, index, LayoutManager::instance()->activeLayout(), groupMode );
    connect( editor, SIGNAL( editingDone( InlineEditorWidget *) ), this, SLOT( editorDone(  InlineEditorWidget *) ) );
    return editor;
}

void Playlist::PrettyItemDelegate::setModelData( QWidget * editor, QAbstractItemModel * model, const QModelIndex &index ) const
{
    Q_UNUSED( model )
    DEBUG_BLOCK

    InlineEditorWidget * inlineEditor = dynamic_cast<InlineEditorWidget *>( editor );
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

    Meta::EditCapability *ec = track->create<Meta::EditCapability>();
    if( !ec || !ec->isEditable() )
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
                {
                    int rating = value.toInt();
                    track->setRating( rating );
                    break;
                }
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
                ec->setYear( value );
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

    editor->setFixedSize( option.rect.size() );
    editor->setGeometry( option.rect );
}

void
Playlist::PrettyItemDelegate::editorDone( InlineEditorWidget * editor )
{
    DEBUG_BLOCK;
    emit commitData( editor );
}



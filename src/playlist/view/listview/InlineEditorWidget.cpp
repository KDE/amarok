/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "InlineEditorWidget.h"

#include "Debug.h"
#include "SvgHandler.h"
#include "playlist/proxymodels/GroupingProxy.h"


#include <kratingwidget.h>
#include <KHBox>
#include <KVBox>

#include <QLabel>
#include <QLineEdit>
#include <QPainter>

using namespace Playlist;

const qreal InlineEditorWidget::ALBUM_WIDTH = 50.0;
const qreal InlineEditorWidget::SINGLE_TRACK_ALBUM_WIDTH = 40.0;
const qreal InlineEditorWidget::MARGIN = 2.0;
const qreal InlineEditorWidget::MARGINH = 6.0;
const qreal InlineEditorWidget::MARGINBODY = 1.0;
const qreal InlineEditorWidget::PADDING = 1.0;

InlineEditorWidget::InlineEditorWidget( QWidget * parent, const QModelIndex &index, PlaylistLayout layout, int groupMode )
 : KVBox( parent )
 , m_index( index )
 , m_layout( layout )
 , m_groupMode( groupMode )
{

    setContentsMargins( 0, 0, 0, 0 );
    int height = 0;

    QFontMetricsF nfm( font() );
    QFont boldfont( font() );
    boldfont.setBold( true );
    QFontMetricsF bfm( boldfont );

    int s_fontHeight = bfm.height();

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

    height = MARGIN * 2 + rowCount * s_fontHeight + ( rowCount - 1 ) * PADDING;
    setFixedHeight( height );
    setFixedWidth( parent->width() );

    createChildWidgets();

}

InlineEditorWidget::~InlineEditorWidget()
{
}

void InlineEditorWidget::createChildWidgets()
{
    DEBUG_BLOCK

    //First create the main KHBox ( for covers and all the rest ) and the top and bottom divider labels
    //To make this weird widget look as close to one of the tracks painted by the delegate as possible

    QPixmap top = The::svgHandler()->renderSvg(
                   "divider_top",
                   width(), 1,
                   "divider_top" );

    QPixmap bottom = The::svgHandler()->renderSvg(
                    "divider_bottom",
                    width(), 1,
                    "divider_bottom" );

    QLabel * topDividerLabel = new QLabel( this );
    topDividerLabel->setPixmap( top );
    topDividerLabel->setGeometry( 0, 0, width(), 1 );
    topDividerLabel->setFixedHeight( 1 );
    
    KHBox * contentsBox = new KHBox( this );

    QLabel * bottomDividerLabel = new QLabel( this );
    bottomDividerLabel->setPixmap( bottom );
    bottomDividerLabel->setGeometry( 0, height(), width(), 1 );
    bottomDividerLabel->setFixedHeight( 1 );

    debug() << "width: " << width();
    //for now, only handle body items
    if( m_groupMode != Body )
        return;

    LayoutItemConfig config = m_layout.body();
    int rowCount = config.rows();

    if ( rowCount == 0 )
        return;

    int rowHeight = height() / rowCount;

    int rowOffsetX = MARGINH;
    //int rowOffsetY = 0;

    int imageSize = height() - MARGIN * 2;
    QRectF nominalImageRect( MARGINH, MARGIN, imageSize, imageSize );

    qreal rowWidth = width() - ( rowOffsetX + MARGINH );

    if ( config.showCover() )
    {
        QModelIndex coverIndex = m_index.model()->index( m_index.row(), CoverImage );
        QPixmap albumPixmap = coverIndex.data( Qt::DisplayRole ).value<QPixmap>();

        if ( albumPixmap.width() > albumPixmap.width() )
            albumPixmap = albumPixmap.scaledToWidth( imageSize );
        else
            albumPixmap = albumPixmap.scaledToHeight( imageSize );

        //offset cover if non square
        QPoint offset = centerImage( albumPixmap, nominalImageRect );
        QRect imageRect( nominalImageRect.x() + offset.x(),
                          nominalImageRect.y() + offset.y(),
                          nominalImageRect.width() - offset.x() * 2,
                          nominalImageRect.height() - offset.y() * 2 );

        QModelIndex emblemIndex = m_index.model()->index( m_index.row(), SourceEmblem );
        QPixmap emblemPixmap = emblemIndex.data( Qt::DisplayRole ).value<QPixmap>();

       
        if ( !albumPixmap.isNull() )
        {
            QPainter painter( &albumPixmap );
            painter.drawPixmap( QRectF( nominalImageRect.x(), nominalImageRect.y() , 16, 16 ), emblemPixmap, QRectF( 0, 0 , 16, 16 ) );

            QLabel * coverLabel = new QLabel( contentsBox );
            coverLabel->setPixmap( albumPixmap );
            coverLabel->setGeometry( imageRect );
            coverLabel->setMaximumSize( imageSize, imageSize );
        }

        rowWidth = width() - ( rowOffsetX + MARGINH + imageSize );

        //rowOffsetX = imageSize + MARGINH + PADDING * 2;
    }

    KVBox * rowsWidget = new KVBox( contentsBox );
    
    rowsWidget->setContentsMargins( 0, 0, 0, 0 );

    for ( int i = 0; i < rowCount; i++ )
    {
        QWidget * rowWidget = new QWidget( rowsWidget );
        rowWidget->setContentsMargins( 0, 0, 0, 0 );
        LayoutItemConfigRow row = config.row( i );
        qreal itemOffsetX = rowOffsetX;

        const int elementCount = row.count();

        QRectF rowBox( itemOffsetX, 0, rowWidth, rowHeight );
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
            int alignment = element.alignment();

            QModelIndex textIndex = m_index.model()->index( m_index.row(), value );
            QString text = textIndex.data( Qt::DisplayRole ).toString();

            qreal itemWidth = 0.0;

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

                    KRatingWidget * ratingWidget = new KRatingWidget( rowWidget );
                    ratingWidget->setGeometry( QRect( currentItemX, 1, itemWidth, rowHeight - 2)  );
                    ratingWidget->setRating( rating );

                } else if ( value == Divider )
                {
                    debug() << "painting divider...";
                    QPixmap left = The::svgHandler()->renderSvg(
                            "divider_left",
                            1, rowHeight ,
                            "divider_left" );

                    QPixmap right = The::svgHandler()->renderSvg(
                            "divider_right",
                            1, rowHeight,
                            "divider_right" );

                    QPainter painter( this );

                    QLabel * leftLabel = new QLabel( rowWidget );
                    leftLabel->setPixmap( left );
                    QLabel * rightLabel = new QLabel( rowWidget );
                    rightLabel->setPixmap( right );
                         
                    if ( alignment & Qt::AlignLeft )
                    {
                        leftLabel->setGeometry( currentItemX, 0, 1, rowHeight );
                        rightLabel->setGeometry( currentItemX + 1, 0, 1, rowHeight );
                    }
                    else if ( alignment & Qt::AlignRight )
                    {
                        leftLabel->setGeometry( currentItemX + itemWidth - 1, 0, 1, rowHeight );
                        rightLabel->setGeometry( currentItemX + itemWidth, 0, 1, rowHeight );
                    }
                    else
                    {
                        int center = currentItemX + ( itemWidth / 2 );
                        leftLabel->setGeometry( center, 0, 1, rowHeight );
                        rightLabel->setGeometry( center + 1, 0, 1, rowHeight );
                    }
                }
                else
                {
                     QLineEdit * edit = new QLineEdit( text, rowWidget );
                     edit->setGeometry( QRect( currentItemX, 0, itemWidth, rowHeight ) );

                     debug() << "creating line edit at " << currentItemX << ", " << 0;
                     debug() << "with size " << itemWidth << ", " << rowHeight;
                }

                currentItemX += itemWidth;
            }

        }
    }
}

QPoint
InlineEditorWidget::centerImage( const QPixmap& pixmap, const QRectF& rect ) const
{
    qreal pixmapRatio = ( qreal )pixmap.width() / ( qreal )pixmap.height();

    qreal moveByX = 0.0;
    qreal moveByY = 0.0;

    if ( pixmapRatio >= 1 )
        moveByY = ( rect.height() - ( rect.width() / pixmapRatio ) ) / 2.0;
    else
        moveByX = ( rect.width() - ( rect.height() * pixmapRatio ) ) / 2.0;

    return QPoint( moveByX, moveByY );
}


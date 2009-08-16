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
#include "playlist/proxymodels/GroupingProxy.h"

#include <kratingwidget.h>

#include <QLineEdit>

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
    int rowOffsetY = 0;

    int imageSize = height() - MARGIN * 2;
    QRectF nominalImageRect( MARGINH, MARGIN, imageSize, imageSize );

    if ( config.showCover() )
    {
        //FIXME
        return;
    }

    int markerOffsetX = nominalImageRect.x();

    for ( int i = 0; i < rowCount; i++ )
    {
        QWidget * rowWidget = new QWidget( this );
        rowWidget->setContentsMargins( 0, 0, 0, 0 );
        LayoutItemConfigRow row = config.row( i );
        qreal itemOffsetX = rowOffsetX;

        const int elementCount = row.count();

        qreal rowWidth = width() - ( rowOffsetX + MARGINH );

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
                    ratingWidget->setGeometry( QRect( currentItemX, rowOffsetY + 1, itemWidth, rowHeight - 2)  );

                } /*else if ( value == Divider )
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
                }*/
                else
                {
                     QLineEdit * edit = new QLineEdit( text, rowWidget );
                     edit->setGeometry( QRect( currentItemX, rowOffsetY, itemWidth, rowHeight ) );

                     debug() << "creating line edit at " << currentItemX << ", " << rowOffsetY;
                     debug() << "with size " << itemWidth << ", " << rowHeight;
                }

                currentItemX += itemWidth;
            }

        }
        rowOffsetY += rowHeight;
    }
}


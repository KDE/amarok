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

#include "playlist/proxymodels/GroupingProxy.h"

using namespace Playlist;

const qreal InlineEditorWidget::ALBUM_WIDTH = 50.0;
const qreal InlineEditorWidget::SINGLE_TRACK_ALBUM_WIDTH = 40.0;
const qreal InlineEditorWidget::MARGIN = 2.0;
const qreal InlineEditorWidget::MARGINH = 6.0;
const qreal InlineEditorWidget::MARGINBODY = 1.0;
const qreal InlineEditorWidget::PADDING = 1.0;

InlineEditorWidget::InlineEditorWidget( QWidget * parent, PlaylistLayout layout, int groupMode )
 : KVBox( parent )
 , m_layout( layout )
{

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

}

InlineEditorWidget::~InlineEditorWidget()
{
}


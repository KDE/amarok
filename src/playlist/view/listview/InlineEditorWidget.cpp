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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "InlineEditorWidget.h"

#include "Debug.h"
#include "PrettyItemDelegate.h"
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
    setSpacing( 0 );
    setAutoFillBackground ( false );
    int height = 0;

    QFontMetricsF nfm( font() );
    QFont boldfont( font() );
    boldfont.setBold( true );
    QFontMetricsF bfm( boldfont );

    int s_fontHeight = bfm.height();

    int rowCount = 1;

    m_headerHeight = 0;

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

    if( groupMode == Head )
        m_headerHeight = ( height * layout.head().rows() ) / rowCount - 1;

    //prevent editor closing when cliking a rating widget or pressing return in a line edit.
    
    setFocusPolicy( Qt::StrongFocus );

    createChildWidgets();
}

InlineEditorWidget::~InlineEditorWidget()
{
}

void InlineEditorWidget::createChildWidgets()
{
    DEBUG_BLOCK

    debug() << "width: " << width();

    //if we are a head item, create a blank widget to make space for the head info

    if ( m_headerHeight != 0 )
    {
        QWidget * headSpacer = new QWidget( this );
        headSpacer->setFixedHeight( m_headerHeight );
    }

    KHBox * trackBox = new KHBox( this );

    LayoutItemConfig config;
    switch(m_groupMode)
    {
        //For now, we don't allow editing of the "head" data, just the body
        case Head:
        case Body:
        case Tail:
            config = m_layout.body();
            break;
        case None:
        default:
            config = m_layout.single();
            break;
    }

    int rowCount = config.rows();

    if ( rowCount == 0 )
        return;

    int rowHeight = height() / rowCount;

    int imageSize = height() - MARGIN * 2;

    if ( config.showCover() )
    {
        //add a small "spacer" widget to offset the cover a little.
        QWidget * coverSpacer = new QWidget( trackBox );
        coverSpacer->setFixedWidth( MARGINH - MARGIN );
        
        QModelIndex coverIndex = m_index.model()->index( m_index.row(), CoverImage );
        QPixmap albumPixmap = coverIndex.data( Qt::DisplayRole ).value<QPixmap>();

        if ( albumPixmap.width() > albumPixmap.width() )
            albumPixmap = albumPixmap.scaledToWidth( imageSize );
        else
            albumPixmap = albumPixmap.scaledToHeight( imageSize );

        QModelIndex emblemIndex = m_index.model()->index( m_index.row(), SourceEmblem );
        QPixmap emblemPixmap = emblemIndex.data( Qt::DisplayRole ).value<QPixmap>();

        if ( !albumPixmap.isNull() )
        {
            QPainter painter( &albumPixmap );
            painter.drawPixmap( QRectF( 0, 0, 16, 16 ), emblemPixmap, QRectF( 0, 0 , 16, 16 ) );

            QLabel * coverLabel = new QLabel( trackBox );
            coverLabel->setPixmap( albumPixmap );
            coverLabel->setGeometry( QRect( 0, 0, height(), height() ) );
            coverLabel->setMaximumSize( height(), height() );
            coverLabel->setMargin ( MARGIN );
        }
    }

    KVBox * rowsWidget = new KVBox( trackBox );
    rowsWidget->setContentsMargins( 0, 0, 0, 0 );
    rowsWidget->setSpacing( 0 );

    for ( int i = 0; i < rowCount; i++ )
    {
        KHBox * rowWidget = new KHBox( rowsWidget );
        rowWidget->setContentsMargins( 0, 0, 0, 0 );

        LayoutItemConfigRow row = config.row( i );

        const int elementCount = row.count();

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
                itemWidth = 100 * size;

                //special case for painting the rating...
                if ( value == Rating )
                {
                    int rating = textIndex.data( Qt::DisplayRole ).toInt();

                    KRatingWidget * ratingWidget = new KRatingWidget( rowWidget );
                    rowWidget->setStretchFactor( ratingWidget, itemWidth );
                    ratingWidget->setRating( rating );
                    ratingWidget->setAttribute( Qt::WA_NoMousePropagation, true );

                    connect( ratingWidget, SIGNAL( ratingChanged( uint ) ), this, SLOT( ratingValueChanged() ) );

                    m_editorRoleMap.insert( ratingWidget, value );

                }
                else if ( value == Divider )
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

                    QPixmap dividerPixmap( 2, rowHeight );
                    dividerPixmap.fill( Qt::transparent );

                    QPainter painter( &dividerPixmap );
                    painter.drawPixmap( 0, 0, left );
                    painter.drawPixmap( 1, 0, right );

                    QLabel * dividerLabel = new QLabel( rowWidget );
                    dividerLabel->setPixmap( dividerPixmap );
                    dividerLabel->setAlignment( element.alignment() );
                    rowWidget->setStretchFactor( dividerLabel, itemWidth );
                }
                else
                {
                     QLineEdit * edit = new QLineEdit( text, rowWidget );
                     rowWidget->setStretchFactor( edit, itemWidth );
                     edit->setAlignment( element.alignment() );

                     connect( edit, SIGNAL( editingFinished() ), this, SLOT( editValueChanged() ) );

                     //check if this is a column that is editable. If not, make the
                     //line edit read only.
                     if ( !editableColumns.contains( value ) )
                     {
                         edit->setReadOnly( true );
                         edit->setDisabled( true );
                     }

                     m_editorRoleMap.insert( edit, value );
                }
            }
        }
    }
}

QPoint
InlineEditorWidget::centerImage( const QPixmap& pixmap, const QRectF& rect ) const
{
    const qreal pixmapRatio = ( qreal )pixmap.width() / ( qreal )pixmap.height();

    qreal moveByX = 0.0;
    qreal moveByY = 0.0;

    if ( pixmapRatio >= 1 )
        moveByY = ( rect.height() - ( rect.width() / pixmapRatio ) ) / 2.0;
    else
        moveByX = ( rect.width() - ( rect.height() * pixmapRatio ) ) / 2.0;

    return QPoint( moveByX, moveByY );
}

void
InlineEditorWidget::paintEvent( QPaintEvent * event )
{
    QPainter painter( this );
    painter.drawPixmap( 0, 0, The::svgHandler()->renderSvgWithDividers( "track", width(), height(), "track" ) );

    //if this is an editor for a head item, use the delegates paintItem function
    //to draw the head part, which currently cannot be edited this way

    if( m_headerHeight != 0 )
    {
        PrettyItemDelegate delegate;
        QStyleOptionViewItem option;
        option.rect = QRect( 0, 0, width(), m_headerHeight );
        delegate.paintItem( m_layout.head(), &painter, option, m_index );
    }

    event->accept();
}

void InlineEditorWidget::editValueChanged()
{
    DEBUG_BLOCK

    QObject * senderObject = sender();

    QLineEdit * edit = dynamic_cast<QLineEdit *>( senderObject );
    if( !edit )
        return;

    int role = m_editorRoleMap.value( edit );
    m_changedValues.insert( role, edit->text() );
}

void InlineEditorWidget::ratingValueChanged()
{
    DEBUG_BLOCK

    QObject * senderObject = sender();

    KRatingWidget * edit = dynamic_cast<KRatingWidget *>( senderObject );
    if( !edit )
        return;

    int role = m_editorRoleMap.value( edit );
    m_changedValues.insert( role, QString::number( edit->rating() ) );
}

QMap<int, QString> InlineEditorWidget::changedValues()
{
    return m_changedValues;
}

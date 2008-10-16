/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jaelevik, Last.fm Ltd <erik@last.fm>                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "draglabel.h"
#include "UnicornCommon.h"

#include <QApplication>
#include <QEvent>
#include <QHelpEvent>
#include <QPainter>
#include <QPalette>
#include <QToolTip>
#include <QDesktopServices>
#include <QDebug>

#include <limits.h>

// Width of the chords at the side ( each of them is chordw / 2 wide )
static const int chordw = 6;
static const int chordMargin = int(0.5f * chordw);
static const int cornerRadius = 4;
static const float s_lineSpacing = 1.0f;
static const int afterHeaderSpace = 4;

DragLabel::DragLabel( QWidget *parent ) :
    QLabel( parent ),
    m_itemsStartAt( 0 ),
    m_lastWidth( -1 ),
    m_lastHfwSize( -1, -1 ),
    m_itemType( -1 ),
    m_answerRect( QRectF( 0, 0, 0, 0 ) ),
    m_hoverPoint( QPoint( -1, -1 ) ),
    m_hoverIndex( -1 ),
    m_hoverable( true ),
    m_selectable( false ),
    m_commas( false ),
    m_justified( false ),
    m_uniformLineHeight( -1 )
{
    setWordWrap( true );
    setMouseTracking( true );

    setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
    setCursor( QCursor( Qt::ArrowCursor ) );

    // Repaint only newly visible items on resize
    setAttribute( Qt::WA_StaticContents );
}

void
DragLabel::append( const QString& text )
{
    DragItem d;
    QString t = m_commas ? text + ',' : text + ' ';
    d.m_text = t;
    calcFontProperties( d );
    m_items << d;
    updateDragLabel();
}

void
DragLabel::clear()
{
    m_items.erase( m_items.begin() + m_itemsStartAt, m_items.end() );
    m_hfwLayout.clear();
    m_lineLayout.clear();

    updateDragLabel();
}

void
DragLabel::clearText()
{
    for ( int i = m_itemsStartAt; i < m_items.count(); ++i )
    {
        m_items[i].m_text = "";
        m_items[i].m_tooltip = "";
        m_items[i].m_url = "";
    }
    updateDragLabel();
}

void
DragLabel::clearSelections()
{
    foreach( DragItem d, m_items )
    {
        d.m_selected = false;
    }

    // No size change so only calling update
    update();
}

void
DragLabel::setBackground( const QPixmap& pixmap )
{
    m_background = pixmap;
}

void
DragLabel::setHeader( const QString& header, const QFont& font )
{
    if ( m_itemsStartAt == 0 )
    {
        DragItem d;
        d.m_text = header;

        //QFont defaultFont = font();
        //defaultFont.setBold( true );
        d.m_font = font;

        calcFontProperties( d, true );

        m_items.insert( 0, d );

        m_itemsStartAt = 1;    
    }
    else
    {
        DragItem& d = m_items[0];
        d.m_text = header;
        d.m_font = font;
        calcFontProperties( d, true );
    }

    updateDragLabel();
}


void
DragLabel::setText( const QString& text )
{ 
    if ( m_items.isEmpty() )
    {
        append( text );
    }
    else
    {
        QString t = m_commas ? text + ',' : text + ' ';
        m_items[0].m_text = t;
        calcFontProperties( m_items[0] );
        updateDragLabel();
    }
}

QString 
DragLabel::text() const
{
    if ( m_items.count() <= m_itemsStartAt )
    {
        return "";
    }
    else
    {
        QString s = m_items.at( m_itemsStartAt ).m_text;
        s.chop( 1 );
        return s;
    }
}

void 
DragLabel::setURL( const QUrl& url )
{
    if ( m_items.isEmpty() )
    {
        DragItem d;
        d.m_url = url;
        m_items << d;
    }
    else
    {
        m_items[0].m_url = url;
    }
}

void 
DragLabel::setFont( const QFont& font )
{
    if ( m_items.isEmpty() )
    {
        DragItem d;
        d.m_font = font;
        m_items << d;
    }
    else
    {
        m_items[0].m_font = font;
    }

    calcFontProperties( m_items[0] );
}

void
DragLabel::setItems( const QStringList& list )
{
    clear();
    foreach ( QString s, list )
    {
        append( s );
    }
    updateDragLabel();
}

QStringList
DragLabel::items()
{
    QStringList l;
    for ( int i = m_itemsStartAt; i < m_items.count(); ++i )
    {
        QString s = m_items[i].m_text;
        s.chop( 1 );
        l << s;
    }
    return l;
}

void
DragLabel::setItemText( int index, const QString& text )
{
    Q_ASSERT( index < ( m_items.count() - m_itemsStartAt ) );

    QString t = m_commas ? text + ',' : text + ' ';
    m_items[m_itemsStartAt + index].m_text = t;
}

void
DragLabel::setItemTooltip( int index, const QString& text )
{
    Q_ASSERT( index < ( m_items.count() - m_itemsStartAt ) );

    m_items[m_itemsStartAt + index].m_tooltip = text;
}

void
DragLabel::setItemFont( int index, QFont font )
{
    Q_ASSERT( index < ( m_items.count() - m_itemsStartAt ) );

    m_items[m_itemsStartAt + index].m_font = font;
    calcFontProperties( m_items[m_itemsStartAt + index] );
}

void
DragLabel::setItemColor( int index, QColor col )
{
    Q_ASSERT( index < ( m_items.count() - m_itemsStartAt ) );

    m_items[m_itemsStartAt + index].m_colour = col;
}

void 
DragLabel::setItemURL( int index, QString url )
{
    Q_ASSERT( index < ( m_items.count() - m_itemsStartAt ) );

    m_items[m_itemsStartAt + index].m_url = url;
}

void 
DragLabel::setItemData( int index, QHash<QString, QString> data )
{
    Q_ASSERT( index < ( m_items.count() - m_itemsStartAt ) );

    m_items[m_itemsStartAt + index].m_dragData = data;
}

QHash<QString, QString>
DragLabel::itemData( int index ) const
{
    Q_ASSERT( index < ( m_items.count() - m_itemsStartAt ) );

    return m_items[m_itemsStartAt + index].m_dragData;
}

void
DragLabel::setItemType( int type )
{
    m_itemType = type;
}

int
DragLabel::itemType() const
{
    return m_itemType;
}

void
DragLabel::setItemsSelectable( bool selectable )
{
    m_selectable = selectable;
    if ( !selectable )
        m_hoverPoint = QPoint( -1, -1 );
}

void 
DragLabel::calcFontProperties( DragItem& d, bool isHeader )
{
    // This defaults to application's default font if not set
    QFontMetrics fm( d.m_font ); 

    QRect rect = fm.boundingRect( d.m_text );

    // Augment to font height as this particular string might be less tall
    if ( fm.height() > rect.height() )
    {
        rect.setHeight( fm.height() );
    }

    // boundingRect sometimes returns negative values so make sure it's at 0, 0
    rect.moveTo( 0, 0 );

    // Move rect to its correct pos and add padding
    if ( isHeader )
    {
        rect.adjust( 0, 0, chordMargin + afterHeaderSpace, 0 );
    }
    else
    {
        rect.adjust( 0, 0, chordMargin * 2, 0 );
    }

    d.m_extent = rect;
    d.m_ascent = fm.ascent();
}

void 
DragLabel::updateDragLabel()
{
    m_lastWidth = -1;
    m_lastHfwSize.setHeight( -1 );
    m_lastHfwSize.setWidth( -1 );
    m_sizeHint.setHeight( -1 );
    m_sizeHint.setWidth( -1 );

    // Tell the layout manager to recalculate itself
    updateGeometry();

    // Redraw the widget
    update();
}

QSize
DragLabel::layoutItems( QList<QRect>& layoutOut, int width ) const
{
    int x = 0;
    int y = 0;
    int lineHeight = 0;
    int firstIdxOfLine = 0;
    int widest = 0;

    int marg = margin();
    int margLeft, margTop, margRight, margBottom;
    getContentsMargins( &margLeft, &margTop, &margRight, &margBottom );
    margLeft += marg;
    margTop += marg;
    margRight += marg;
    margBottom += marg;

    width = width - margLeft - margRight;
    y = margTop;
    x = margLeft;

    for ( int i = 0; i < m_items.count(); i++ )
    {
        // Find out how much space the pill needs
        QRect textRect = m_items[i].m_extent;

        // Use the uniform height if we have one, otherwise use the height of the widget font
        int itemHeight = m_uniformLineHeight > 0 ? m_uniformLineHeight : textRect.height();
        if ( itemHeight > lineHeight )
        {
            lineHeight = itemHeight;
        }

        if ( textRect.width() > widest )
        {
            widest = textRect.width();
        }

        // Move rect to its correct pos
        textRect.moveTo( x, y );

        bool tooBigForCurLine = textRect.width() > ( width - x );
        bool firstThingToBeDrawn = i == 0;

        // Do we need a newline?
        if ( tooBigForCurLine && !firstThingToBeDrawn && wordWrap() )
        {
            baseAlign( layoutOut, firstIdxOfLine, i - 1, lineHeight );
            if ( m_justified )
            {
                justify( layoutOut, firstIdxOfLine, i - 1, width );
            }

            y += lineHeight + int(s_lineSpacing);
            x = margLeft;
            textRect.moveTo( x, y );

            // Reset line height to height of this item
            lineHeight = itemHeight;

            firstIdxOfLine = i;
        }

        layoutOut << textRect;

        x += textRect.width();
    }

    if ( ( m_items.count() - firstIdxOfLine ) > 0 )
    {
        baseAlign( layoutOut, firstIdxOfLine, m_items.count() - 1, lineHeight );
    }
    y += lineHeight;
    y += margBottom;

    int actualWidth;
    if ( width == INT_MAX )
    {
        x += margRight;
        actualWidth = x;
    }
    else
    {
        actualWidth = ( widest > width ) ? widest : width;
    }

    QSize ret( actualWidth, y );

    return ret;
}

void
DragLabel::baseAlign( QList<QRect>& layoutOut, int startIdx, int endIdx, int lineHeight ) const
{
    // Find tallest item in line
    int maxHeight = 0;
    int tallest = -1;
    for ( int i = startIdx; i <= endIdx; ++i )
    {
        int height = m_items[i].m_extent.height();
        if ( height > maxHeight )
        {
            maxHeight = height;
            tallest = i;
        }
    }

    int maxAscent = m_items[tallest].m_ascent;
    int distToBottom = lineHeight - m_items[tallest].m_extent.bottom() - 1;

    Q_ASSERT( maxAscent != -1 );

    // Move all items down to align their baseline with the tallest item
    for ( int j = startIdx; j <= endIdx; ++j )
    {
        int pushDist = maxAscent - m_items[j].m_ascent;
        Q_ASSERT( pushDist >= 0 );

        pushDist += distToBottom;
        layoutOut[j].translate( 0, pushDist );
    }
}

void
DragLabel::justify( QList<QRect>& layoutOut, int startIdx, int endIdx, int width ) const
{
    int combinedWidth = 0;
    for ( int i = startIdx; i <= endIdx; ++i )
    {
        combinedWidth += m_items[i].m_extent.width();
    }

    int space = width - combinedWidth;
    int nGaps = qMax( endIdx - startIdx, 1 );
    int gapSpace = space / nGaps;

    int cnt = 1;
    for ( int j = startIdx + 1; j <= endIdx; ++j, ++cnt )
    {
        layoutOut[j].translate( gapSpace * cnt, 0 );
    }
}

int
DragLabel::heightForWidth( int w ) const
{
    // This function is called by the layout manager instead of sizeHint when we
    // have specified that word wrap should be used.

    // ensurePolished?

    if ( m_lastWidth == -1 || w != m_lastWidth )
    {
        m_lastWidth = w;
        m_hfwLayout.clear();
        m_lastHfwSize = layoutItems( m_hfwLayout, w );
    }

    return m_lastHfwSize.height();
}

QSize
DragLabel::sizeHint() const
{
    // Work out width if all items are put on one line and return that.
    // Only used as a default when we don't have word wrapping enabled.
    // This will also be used if specifying Fixed as the size policy.

    if ( !m_sizeHint.isValid() )
    {
        m_lineLayout.clear();
        m_sizeHint = layoutItems( m_lineLayout, INT_MAX );
    }

    return m_sizeHint;
}

QSize
DragLabel::minimumSizeHint() const
{
    // A layout will never size us smaller than what we return here
    if ( !m_sizeHint.isValid() )
    {
        m_lineLayout.clear();
        m_sizeHint = layoutItems( m_lineLayout, INT_MAX );
    }

    return QSize( 0, m_sizeHint.height() );
}

bool
DragLabel::event( QEvent* event )
{
    if ( event->type() == QEvent::ToolTip )
    {
        QHelpEvent* helpEvent = static_cast<QHelpEvent *>( event );
        QPoint hoverPos = helpEvent->pos();

        int index = itemAt( hoverPos );

        if ( index != -1 )
        {
            //QToolTip::showText( helpEvent->globalPos(), m_items[index].m_tooltip, this, m_hfwLayout.at( index ) );
            QToolTip::showText( helpEvent->globalPos(), m_items[index].m_tooltip );
        }
        else
        {
            QToolTip::hideText();
        }
    }

    return QLabel::event( event );
}

void
DragLabel::paintEvent( QPaintEvent * event )
{
    //qDebug() << "paintevent rect: " << event->rect();

    QPainter painter( this );

    if ( !m_background.isNull() )
    {
        painter.drawTiledPixmap( event->rect(), m_background );
    }

    m_answerRect = QRectF( 0, 0, 0, 0 );
    m_hoverIndex = -1;

    if ( !anythingToDraw() )
    {
        return;
    }

    // The space we've been given by the layout system
    QRect space = contentsRect();

    // Get us a layout
    int w = space.width();
    if ( m_lastWidth == -1 || w != m_lastWidth )
    {
        m_lastWidth = w;
        m_hfwLayout.clear();
        m_lastHfwSize = layoutItems( m_hfwLayout, w );
    }

    for ( int i = 0; i < m_items.count(); ++i )
    {
        DragItem& item = m_items[i];

        painter.setFont( item.m_font );
        if ( item.m_colour.isValid() )
        {
            painter.setPen( item.m_colour );
            painter.setBrush( item.m_colour );
        }
        else
        {
            painter.setPen( palette().text().color() );
            painter.setBrush( palette().text().color() );
        }

        QString text = item.m_text;

        // Remove the trailing comma/space from the last item
        if ( i == ( m_items.count() - 1 ) )
        {
            text.chop( 1 );
        }

        QRect itemRect = m_hfwLayout[i];

        // Crop itemRect to the width of our boundaries
        bool cropped = false;
        if ( space.width() < itemRect.width() )
        {
            itemRect.setWidth( space.width() );
            cropped = true;
        }

        // According to Alberto Garcia, this fixes the draglabel painting issue on Linux.
        QRect textRect( itemRect.topLeft() + QPoint( chordMargin, 0 ),
                        itemRect.bottomRight() ); // - QPoint( chordMargin, 0 ) );

        if ( cropped )
        {
            QFontMetrics fm = painter.fontMetrics();
            text = fm.elidedText( text, Qt::ElideRight, textRect.width() );
        }

        bool isHeader = i == 0 && m_itemsStartAt == 1;

        // Draw pill if we're hovered or selected
        if ( ( itemRect.contains( m_hoverPoint ) || item.m_selected ) && !isHeader )
        {
            if ( itemRect.contains( m_hoverPoint ) )
            {
                m_hoverIndex = i;
                m_answerRect = itemRect;
            }

            #ifdef WIN32
                // Looks shit on Win with aa on
                painter.setRenderHint( QPainter::Antialiasing, false );
            #else
                // Looks shit on Mac with aa off
                painter.setRenderHint( QPainter::Antialiasing, true );
            #endif

            // On hover, we remove the comma or trailing space
            if ( i != ( m_items.count() - 1 ) && !cropped )
            {
                text.chop( 1 );
            }

            QColor highlight( 0xb4, 0xc2, 0xd4 );
            QColor background( 0xd6, 0xdf, 0xec );

            // Hannah doesn't like the anti-aliasing on the Mac
            #ifdef Q_WS_MAC
                painter.setPen( background );
            #else
                painter.setPen( highlight );
            #endif
            painter.setBrush( background );

            // Get new rect for text without comma
            QRect pillRect = painter.fontMetrics().boundingRect( itemRect, Qt::AlignLeft, text );
            pillRect.adjust( 0, 0, chordMargin * 2, 0 );

            // Paint the rectangle
            QRect r( pillRect.topLeft() + QPoint( 1, 1 ),
                     pillRect.bottomRight() + QPoint( -1, -1 ) );

            painter.drawRoundRect( r, roundnessForLength( r.width() ),
                                      roundnessForLength( r.height() ) );

            // We want white text on hovering
            //painter.setPen( Qt::white );
            //painter.setBrush( Qt::white );
            painter.setPen( item.m_colour );
        }

        // Draw text
        painter.setRenderHint( QPainter::Antialiasing, true );
        painter.setRenderHint( QPainter::TextAntialiasing, true );
        painter.drawText( textRect, text );

    }

    // Highly dubious!
    if ( sizePolicy().verticalPolicy() == QSizePolicy::MinimumExpanding )
    {
        setMinimumHeight( m_lastHfwSize.height() );
    }

    // set the empty bottom space as our cached answerRect
    if ( !m_answerRect.width() )
        m_answerRect = QRectF( 0, m_lastHfwSize.height(), 1, 1 );
}

bool
DragLabel::anythingToDraw()
{
    if ( m_items.count() == 0 || ( m_items.count() == 1 && m_itemsStartAt == 1 ) )
    {
        return false;
    }

    bool haveText = false;
    foreach( DragItem item, m_items )
    {
        if ( item.m_text != "" && item.m_text != " " )
        {
            haveText = true;
            break;
        }
    }

    return haveText;
}

int
DragLabel::roundnessForLength( int len )
{
    if ( len == 0 ) return 0;

    int round = (int)( ( (float)cornerRadius / len ) * 100 );
    round = qMin( 99, round );
    round = qMax( 1, round );

    //qDebug() << len << " -> " << round;

    return round;
}

int
DragLabel::itemAt( const QPoint& pos )
{
    for ( int i = 0; i < m_hfwLayout.size(); ++i )
    {
        const QRect& itemRect = m_hfwLayout.at( i );
        if ( itemRect.contains( pos ) )
        {
            return i;
        }
    }
    return -1;
}

void
DragLabel::leaveEvent( QEvent* /*event*/ )
{
    m_answerRect = QRectF( 0, 0, 0, 0 );
    m_hoverPoint = QPoint( -1, -1 );
    update();

    emit urlHovered( "" );
}

void
DragLabel::mousePressEvent( QMouseEvent *event )
{
    if ( !m_selectable && m_hoverIndex >= 0 )
        QLabel::mousePressEvent( event );

    if ( event->button() == Qt::LeftButton )
    {
        m_dragStartPosition = event->pos();
        //setCursor( QCursor( Qt::ClosedHandCursor ) );
    }

}


void
DragLabel::mouseReleaseEvent( QMouseEvent *event )
{
    if ( m_hoverIndex >= 0 )
    {
        if ( m_selectable )
        {
            m_items[m_hoverIndex].m_selected = !m_items[m_hoverIndex].m_selected;
            update();
        }
        else
        {
            if ( ( event->pos() - m_dragStartPosition ).manhattanLength() > QApplication::startDragDistance() )
                return;

            qDebug() << "Opening url:" << m_items[m_hoverIndex].m_url;

            if ( !m_items[m_hoverIndex].m_url.isEmpty() )
            {
                #ifndef Q_WS_WIN
                    QDesktopServices::openUrl( QUrl::fromEncoded( m_items[m_hoverIndex].m_url.toString().toUtf8() ) );
                #else
                    QDesktopServices::openUrl( m_items[m_hoverIndex].m_url );
                #endif
            }
        }

        //setCursor( QCursor( Qt::ArrowCursor ) );

        emit clicked( m_hoverIndex );
    }
}


void
DragLabel::mouseMoveEvent( QMouseEvent *event )
{
    QLabel::mouseMoveEvent( event );

    if ( !m_hoverable )
        return;

    if ( !m_answerRect.contains( event->pos() ) )
    {
        // We've just moved off/on a pill
        m_hoverPoint = event->pos();
        update();
    }

    // Stuff from here on is stuff we need to update for each nudge of the mouse
    if ( m_hoverIndex < 0 )
    {
        setCursor( QCursor( Qt::ArrowCursor ) );

        emit urlHovered( "" );
    }
    else
    {
        //if ( !( event->buttons() & Qt::LeftButton ) )
        //{
        //    setCursor( QCursor( Qt::OpenHandCursor ) );
        //}

        setCursor( QCursor( Qt::PointingHandCursor ) );

        // Emit hovered over url
        QString url = m_items[m_hoverIndex].m_url.toString();
        if ( !url.isEmpty() )
        {
            emit urlHovered( url );
        }
    }

    // Early out if left button isn't pressed or we're not over a pill
    if ( !( event->buttons() & Qt::LeftButton ) || m_hoverIndex < 0 )
        return;

    // Early out if drag not long enough yet
    if ( ( event->pos() - m_dragStartPosition ).manhattanLength() < QApplication::startDragDistance() )
        return;

    QString anchor = m_items[m_hoverIndex].m_text;

    anchor = anchor.trimmed();
    if ( anchor.endsWith( ',' ) )
    {
        anchor.chop( 1 );
    }

    if ( !anchor.isEmpty() )
    {
        QDrag* drag = new QDrag( this );
//         qDebug() << "New drag with type" << itemType();

        QMimeData *mimeData = new QMimeData();
        mimeData->setText( anchor );
        mimeData->setData( "item/type", QByteArray::number( itemType() ) );

        QHash<QString, QString> data = m_items[m_hoverIndex].m_dragData;
        if ( data.count() )
        {
            for ( int i = 0; i < data.count(); i++ )
            {
                //qDebug() << "Setting data" << data.keys().at( i ) << data.values().at( i );
                mimeData->setData( QString( "item/%1" ).arg( data.keys().at( i ) ), data.values().at( i ).toUtf8() );
            }
        }
        else
        {
            switch( itemType() )
            {
                case UnicornEnums::ItemArtist:
                    mimeData->setData( "item/artist", anchor.toUtf8() );
                    break;

                case UnicornEnums::ItemTag:
                    mimeData->setData( "item/tag", anchor.toUtf8() );
                    break;

                case UnicornEnums::ItemUser:
                    mimeData->setData( "item/user", anchor.toUtf8() );
                    break;

                case UnicornEnums::ItemStation:
                    mimeData->setData( "item/station", anchor.toUtf8() );
                    break;
            }
        }

        QPainter painter;
        QPixmap pixmap( painter.fontMetrics().width( anchor ) + 16, painter.fontMetrics().height() + 4 );
        QRect rect( 0, 0, pixmap.width() - 1, pixmap.height() - 1 );

        painter.begin( &pixmap );
        painter.setBackgroundMode( Qt::OpaqueMode );

        painter.setBrush( Qt::white );
        painter.setPen( Qt::black );
        painter.drawRect( rect );

        painter.setPen( Qt::black );
        painter.drawText( rect, Qt::AlignCenter, anchor );
        painter.end();

        drag->setMimeData( mimeData );
        drag->setPixmap( pixmap );

        Qt::DropAction dropAction = drag->start( Qt::CopyAction );

        Q_UNUSED( dropAction )
    }
}


void
DragLabel::setItemSelected( const QString& text, bool selected, bool emitSignal )
{
    int index = -1;
    for( int i = m_itemsStartAt; i < m_items.count(); ++i )
    {
        if ( m_items[i].m_text == text )
        {
            index = i;
        }
    }
	Q_ASSERT(index >= 0);

    setItemSelected( index - m_itemsStartAt, selected, emitSignal );
}


void
DragLabel::setItemSelected( int index, bool selected, bool emitSignal )
{
    if ( index >= 0 && index < m_items.count() )
    {
        m_items[m_itemsStartAt + index].m_selected = selected;

        update();
        if ( emitSignal )
            emit clicked( index );
    }
}

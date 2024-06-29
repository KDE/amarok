/****************************************************************************************
 * Copyright (c) 2008 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2009 Thomas Luebking <thomas.luebking@web.de>                          *
 * Copyright (c) 2009 Daniel Dewald <Daniel.Dewald@time-shift.de>                       *
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

#include "TokenPool.h"

#include <QApplication>

#include <QMouseEvent>
#include <QMimeData>
#include <QDrag>

#include "core/support/Debug.h"

TokenPool::TokenPool( QWidget *parent )
    : QListWidget( parent )
{
    setAcceptDrops( true );
    setWrapping( true );
    setResizeMode( QListView::Adjust );
}

void
TokenPool::addToken( Token * token )
{
    token->setParent( this );
    token->setVisible( false );

    QListWidgetItem *item = new QListWidgetItem( token->icon(), token->name() );
    if( token->hasCustomColor() ) // don't override the default tooltip color if possible. This very easily produces black test on black tooltip background
    {
        item->setData( Qt::ForegroundRole, token->textColor() );
        item->setToolTip( QStringLiteral("<font color=\"") + token->textColor().name() + QStringLiteral("\">") + token->name() + QStringLiteral("</font>") );
    }
    else
    {
        item->setToolTip( token->name() );
    }
    addItem( item );

    token->setParent( this );
    token->hide();
    m_itemTokenMap.insert( item, token );
}

QSize
TokenPool::sizeHint() const
{
    int h = iconSize().height();
    if (h <= 0) {
        h = style()->pixelMetric(QStyle::PM_SmallIconSize, nullptr, this);
    }

    // we are planning the size for three columns of token text
    // with eight rows (note: we might get less than eight rows if because
    // of the space the border and the scroll bar uses).
    return QSize(fontMetrics().horizontalAdvance(QLatin1String("Artist's Initial")) * 3,
                 h * 8 );
}

// Executed on doubleclick of the TokenPool, emits signal onDoubleClick( QString )
// that connects to TokenLayoutWidget::addToken( QString )
void
TokenPool::mouseDoubleClickEvent( QMouseEvent *event )
{
    QListWidgetItem *tokenItem = itemAt( event->pos() );
    if( tokenItem )
        Q_EMIT onDoubleClick( m_itemTokenMap.value( tokenItem ) );
}

//Executed on mouse press, handles start of drag.
void
TokenPool::mousePressEvent( QMouseEvent *event )
{
    if( event->button() == Qt::LeftButton )
        m_startPos = event->pos();            //store the start position
    QListWidget::mousePressEvent( event );    //feed it to parent's event
}

//Executed on mouse move, handles start of drag.
void
TokenPool::mouseMoveEvent( QMouseEvent *event )
{
    if( event->buttons() & Qt::LeftButton )
    {
        int distance = ( event->pos() - m_startPos ).manhattanLength();
        if ( distance >= QApplication::startDragDistance() )
            performDrag();
    }
    QListWidget::mouseMoveEvent( event );
}

void
TokenPool::dragEnterEvent( QDragEnterEvent *event )
{
    QObject *source = event->source();
    if( source != this && event->mimeData()->hasFormat( Token::mimeType() ) )
    {
        event->setDropAction( Qt::MoveAction );
        event->accept();
    }
}

void
TokenPool::dropEvent( QDropEvent *event )
{
    Q_UNUSED( event )
    //does nothing, I want the item to be deleted and not dragged here
}

//Handles the creation of a QDrag object that carries the (text-only) QDataStream from an item in TokenPool
void
TokenPool::performDrag()
{
    QListWidgetItem *item = currentItem();

    if( item )
    {
        Token *token = m_itemTokenMap.value( item );

        QDrag *drag = new QDrag( this );
        drag->setMimeData( token->mimeData() );

        // -- icon for pointer
        // since the TokenPools tokens are invisible we need to resize them before drawing
        // in the pixmap buffer
        token->resize( token->sizeHint() );

        // now draw in the pixmap buffer
        QPixmap pixmap( token->size() );
        token->render( &pixmap );
        drag->setPixmap( pixmap );
        drag->setHotSpot ( pixmap.rect().center() );

        drag->exec( Qt::MoveAction | Qt::CopyAction, Qt::CopyAction );
    }
}


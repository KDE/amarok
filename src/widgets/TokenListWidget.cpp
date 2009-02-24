/******************************************************************************
 * Copyright (C) 2008 Teo Mrnjavac <teo.mrnjavac@gmail.com>                   *
 *               2009 Seb Ruiz <ruiz@kde.org>                                 *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/
#include "TokenListWidget.h"

#include <KApplication>

#include <QMouseEvent>


TokenListWidget::TokenListWidget( QWidget *parent )
    : KListWidget( parent )
{
    setAcceptDrops( true );
}

void
TokenListWidget::addToken( Token * token )
{

    QListWidgetItem *item = new QListWidgetItem( token->icon().pixmap( 48, 48 ), token->name() );
    addItem( item );

    m_itemTokenMap.insert( item, token );
}

QString
TokenListWidget::mimeType() const
{
    return m_mimeType;
}

void
TokenListWidget::setMimeType( const QString& mimeType )
{
    m_mimeType = mimeType;
}

// Executed on doubleclick of the TokenListWidget, emits signal onDoubleClick( QString )
// that connects to FilenameLayoutWidget::addToken( QString )
void
TokenListWidget::mouseDoubleClickEvent( QMouseEvent *event )
{
    QListWidgetItem *tokenItem = itemAt( event->pos() );
    if( tokenItem )
        emit onDoubleClick( m_itemTokenMap.value( tokenItem ) ); //token->name() << token->iconName() << token->value()
}

//Executed on mouse press, handles start of drag.
void
TokenListWidget::mousePressEvent( QMouseEvent *event )
{
    if( event->button() == Qt::LeftButton )
        m_startPos = event->pos();            //store the start position
    KListWidget::mousePressEvent( event );    //feed it to parent's event
}

//Executed on mouse move, handles start of drag.
void
TokenListWidget::mouseMoveEvent( QMouseEvent *event )
{
    if( event->buttons() & Qt::LeftButton )
    {
        int distance = ( event->pos() - m_startPos ).manhattanLength();
        if ( distance >= KApplication::startDragDistance() )
            performDrag( event );
    }
    KListWidget::mouseMoveEvent( event );
}

//This doesn't do much since TokenListWidget doesn't accept objects.
void
TokenListWidget::dragEnterEvent( QDragEnterEvent *event )
{
    QWidget *source = qobject_cast<QWidget *>( event->source() );
    if ( source && source != this )
    {
        event->setDropAction( Qt::MoveAction );
        event->accept();
    }
}

//Same as above.
void
TokenListWidget::dragMoveEvent( QDragMoveEvent *event )        //overrides QListWidget's implementation
{
    QWidget *source = qobject_cast<QWidget *>( event->source() );
    if ( source && source != this )
    {
        event->setDropAction( Qt::MoveAction );
        event->accept();
    }
}

//Same as above.
void
TokenListWidget::dropEvent( QDropEvent *event )
{
    Q_UNUSED( event )
    //does nothing, I want the item to be deleted and not dragged here
}

//Handles the creation of a QDrag object that carries the (text-only) QDataStream from an item in TokenListWidget
void
TokenListWidget::performDrag( QMouseEvent *event )
{
    QListWidgetItem *item = currentItem();

    if( item )
    {
        Token *token = m_itemTokenMap.value( item );
        QByteArray itemData;

        QDataStream dataStream( &itemData, QIODevice::WriteOnly );
        dataStream << token->name() << token->iconName() << token->value() << QPoint( event->pos() - rect().topLeft() );
        
        QMimeData *mimeData = new QMimeData;
        mimeData->setData( m_mimeType, itemData );
        
        QDrag *drag = new QDrag( this );
        drag->setMimeData( mimeData );
        
        //TODO: set a pointer for the drag, like this: drag->setPixmap( QPixmap("foo.png" ) );
        drag->exec( Qt::MoveAction | Qt::CopyAction, Qt::CopyAction );
    }
}

#include "TokenListWidget.moc"

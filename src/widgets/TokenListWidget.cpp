/******************************************************************************
 * Copyright (C) 2008 Teo Mrnjavac <teo.mrnjavac@gmail.com>                   *
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
#include "FilenameLayoutDialog.h"
#include "Debug.h"

#include <KApplication>
#include <KDialog>

#include <QMouseEvent>


TokenListWidget::TokenListWidget( QWidget *parent )
    : KListWidget( parent )
{
    setAcceptDrops( true );

    //filenameLayoutWidget = qobject_cast< FilenameLayoutDialog * >( parent )->filenameLayout;    //omg filenameLayoutDialog is NOT a parent of filenameLayoutWidget, the parent is a stupid QWidget

    addItem( new QListWidgetItem( KIcon( "placeholder.svg" ), i18n( "Track" ) ) );
    addItem( new QListWidgetItem( KIcon( "placeholder.svg" ), i18n( "Title" ) ) );
    addItem( new QListWidgetItem( KIcon( "placeholder.svg" ), i18n( "Artist" ) ) );
    addItem( new QListWidgetItem( KIcon( "placeholder.svg" ), i18n( "Composer" ) ) );
    addItem( new QListWidgetItem( KIcon( "placeholder.svg" ), i18n( "Year" ) ) );
    addItem( new QListWidgetItem( KIcon( "placeholder.svg" ), i18n( "Album" ) ) );
    addItem( new QListWidgetItem( KIcon( "placeholder.svg" ), i18n( "Comment" ) ) );
    addItem( new QListWidgetItem( KIcon( "placeholder.svg" ), i18n( "Genre" ) ) );
    addItem( new QListWidgetItem( KIcon( "placeholder.svg" ), i18n( " _ ") ) );
    addItem( new QListWidgetItem( KIcon( "placeholder.svg" ), i18n( " - " ) ) );
    addItem( new QListWidgetItem( KIcon( "placeholder.svg" ), i18n( " . " ) ) );
    addItem( new QListWidgetItem( KIcon( "placeholder.svg" ), QString("<space>") ) );
    //addItem( new QListWidgetItem( KIcon("placeholder.svg"), QString( "Track" ) ) );

    //setViewMode( QListView::ListMode );   //I try to let this be handled by .ui
    //setFlow( QListView::LeftToRight );
}

//Executed on doubleclick of the TokenListWidget, emits signal onDoubleClick( QString ) that connects to FilenameLayoutWidget::addToken( QString )
void
TokenListWidget::mouseDoubleClickEvent( QMouseEvent *event )
{
    QListWidgetItem *token = itemAt( event->pos() );
    debug()<<"Double-clicked, gonna add token!";
    emit onDoubleClick( token->text() );
}

//Executed on mouse press, handles start of drag.
void
TokenListWidget::mousePressEvent( QMouseEvent *event )
{
    if ( event->button() == Qt::LeftButton )
        m_startPos = event->pos();            //store the start position
    debug()<<"Mouse pressed, got start position.";
    KListWidget::mousePressEvent( event );    //feed it to parent's event
}

//Executed on mouse move, handles start of drag.
void
TokenListWidget::mouseMoveEvent( QMouseEvent *event )
{
    if ( event->buttons() & Qt::LeftButton )
    {
        int distance = ( event->pos() - m_startPos ).manhattanLength();
        debug()<<"Mouse moved, calculated distance from start position.";
        if ( distance >= KApplication::startDragDistance() )
        {
            performDrag( event );
        }
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
    if ( item )
    {
        QByteArray itemData;
        QDataStream dataStream( &itemData, QIODevice::WriteOnly );
        dataStream << item->text() << QPoint( event->pos() - rect().topLeft() );
        QMimeData *mimeData = new QMimeData;
        mimeData->setData( "application/x-amarok-tag-token", itemData );    //setText( item->text() );
        QDrag *drag = new QDrag( this );
        drag->setMimeData( mimeData );
        debug() << "I am dragging from the token pool, performing drag";
        //TODO: set a pointer for the drag, like this: drag->setPixmap( QPixmap("foo.png" ) );
        drag->exec( Qt::MoveAction | Qt::CopyAction, Qt::CopyAction );
    }
}


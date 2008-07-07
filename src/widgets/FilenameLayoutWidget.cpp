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

#include "FilenameLayoutWidget.h"
#include "Debug.h"

#include <KApplication>
#include <KPushButton>

#include <QMouseEvent>
#include <QByteArray>
#include <QDataStream>
#include <QDragMoveEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QtGui>

FilenameLayoutWidget::FilenameLayoutWidget( QWidget *parent )
    : QFrame( parent )
{
    setAcceptDrops( true );
    layout = new QHBoxLayout;
    layout->setSpacing( 1 );
    setLayout( layout );
    backText = new QLabel;
    backText->setText( i18n( "<div align=center><i>Drag tokens here to define a filename scheme.</i></div>" ) );
    layout->addWidget( backText );
    tokenCount = 0;     //how many tokens have I built, need this to assign unique IDs
}

void
FilenameLayoutWidget::slotAddToken()
{
    this->addToken( i18n( "TOKEN" ) );
}

void
FilenameLayoutWidget::addToken( QString text ){
    if( backText->isVisible() )
    {
        backText->hide();
    }

    tokenCount++;
    Token *token = new Token( text, this );

    layout->addWidget( token );
    token->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    token->show();
}


void
FilenameLayoutWidget::dragEnterEvent( QDragEnterEvent *event )        //overrides QListWidget's implementation. this is when the drag becomes droppable
{
    QWidget *source = qobject_cast<QWidget *>( event->source() );
    if ( source && source != this )
    {
        event->setDropAction( Qt::CopyAction );
        event->accept();
    }
    else if ( source && source == this )
    {
        event->setDropAction( Qt::MoveAction );
        event->accept();
    }
}

void
FilenameLayoutWidget::dragMoveEvent( QDragMoveEvent *event )          //need to override QListWidget
{
    QWidget *source = qobject_cast<QWidget *>( event->source() );     //same as in dragEnterEvent
    if ( source && source != this )
    {
        event->setDropAction( Qt::CopyAction );
        event->accept();
    }
    else if ( source && source == this )
    {
        event->setDropAction( Qt::MoveAction );
        event->accept();
    }
}

void FilenameLayoutWidget::dropEvent( QDropEvent *event )
{
    QWidget *source = qobject_cast<QWidget *>( event->source() );     //not sure how to handle this
    if ( source && source != this )
    {
        debug() << "I'm dragging from the token pool";
        addToken( event->mimeData()->text() );      //TODO: eliminate the mimeData->setText and ->text calls in favor of grabbing the text from mimeData
        event->setDropAction( Qt::CopyAction );
        event->accept();
    }
    else if ( source && source == this )
    {
        debug() << "I'm dragging from the layout widget";
        addToken( event->mimeData()->text() );
        event->setDropAction( Qt::MoveAction );
        event->accept();
    }
}

unsigned int FilenameLayoutWidget::getTokenCount()
{
    return tokenCount;
}



void
FilenameLayoutWidget::mouseMoveEvent( QMouseEvent *event )
{
    if ( event->buttons() & Qt::LeftButton )
    {
        int distance = ( event->pos() - startPos ).manhattanLength();
        if ( distance >= KApplication::startDragDistance() )
        {
            performDrag( event );
        }
    }
}

void
FilenameLayoutWidget::mousePressEvent( QMouseEvent *event )
{
    if ( event->button() == Qt::LeftButton )
        startPos = event->pos();
}

void
FilenameLayoutWidget::performDrag( QMouseEvent *event )
{
    //transfer of QByteData, not text --thank you Fridge Magnet example from Qt doc
    Token *child = dynamic_cast< Token * >( childAt( event->pos() ) );
    if ( !child )
        return;
    QByteArray itemData;
    QDataStream dataStream( &itemData, QIODevice::WriteOnly );
    dataStream << child->text(); // << QPoint( event->pos() - child->rect().topLeft() -child.pos() );       //I may need the QPoint of the start sooner or later
    QMimeData *mimeData = new QMimeData;
    mimeData->setData( "application/x-amarok-tag-token", itemData );
    mimeData->setText( child->text() );
    QDrag *drag = new QDrag( this );
    drag->setMimeData( mimeData );
    drag->setHotSpot( event->pos() - child->rect().topLeft() - child->pos() );        //I grab the initial position of the item I'm dragging
    
    drag->setPixmap( QPixmap::grabWidget( child ) );       //need to get pixmap from widget

    child->hide();

    drag->exec(Qt::MoveAction | Qt::CopyAction, Qt:: CopyAction);
}

//starts implementation of Token : QLabel

Token::Token( const QString &string, QWidget *parent )
    : QLabel( parent )
{
    myCount = qobject_cast<FilenameLayoutWidget *>( parent )->getTokenCount();
    //TODO: resize the labels according to the text size, smth like this:

    setText( string );
    setTokenString( string );
    setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
    setStyleSheet( "Token {\
        color: palette( Base );\
        background-color: qlineargradient( x1: 0,\
                                           y1: 0,\
                                           x2: 1,\
                                           y2: 1,\
                                           stop: 0 white,\
                                           stop: 0.4 gray,\
                                           stop: 1 blue );\
    }" );

    QFontMetrics metric( font() );
    QSize size = metric.size( Qt::TextSingleLine, text() );
    setMinimumSize( size );
}

void
Token::setTokenString( const QString &string )
{
    tokenString = string;
}
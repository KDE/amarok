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

#include <KPushButton>

#include <QMouseEvent>
#include <QDragMoveEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QtGui>

FilenameLayoutWidget::FilenameLayoutWidget(QWidget *parent) : QFrame(parent)
{
    setAcceptDrops(true);
    layout = new QHBoxLayout;
    layout->setSpacing(1);
    setLayout(layout);
    backText = new QLabel;
    backText->setText("<i>Drag tokens here to define a filename scheme.</i>");
    layout->addWidget(backText);
    
}

void
FilenameLayoutWidget::addToken(){
    if(backText->isVisible())
    {
        backText->hide();
    }
    
    Token *token = new Token(this);
    
    layout->addWidget(token);
    //TODO: something like token->setDragEnabled(true);
    token->show();
    
    connect(token, SIGNAL(signalMousePressEvent(QMouseEvent *event)),      //why doesn't it pick up my signal declaration?
            this, SLOT(slotMousePressEvent(QMouseEvent *event)));
}

void
FilenameLayoutWidget::slotMousePressEvent(QMouseEvent *event)
{
    //TODO: this is when I store the start position to start the drag on mouseMoveEvent when the mouse has moved enough
              /*something like
              if (event->button() == Qt::LeftButton)
              startPos = event->pos();            //store the start position
              QFrame::mousePressEvent(event);    //feed it to parent's or token's event
              */
}

void
FilenameLayoutWidget::mouseMoveEvent(QMouseEvent *event)
{
    //TODO: this has to become a slot of FilenameLayoutWidget that responds to the mouseMoveEvent raised by a Token, like slotMousePressEvent(QMouseEvent *event)
    /*if (event->buttons() & Qt::LeftButton) {
        int distance = (event->pos() - startPos).manhattanLength();
        if (distance >= KApplication::startDragDistance()){
            performDrag();
        }
    }
    KListWidget::mouseMoveEvent(event);     //feed it to Token's event? not sure
    */
}

void
FilenameLayoutWidget::dragEnterEvent(QDragEnterEvent *event)        //overrides QListWidget's implementation. this is when the drag becomes droppable
{
    QWidget *source = qobject_cast<QWidget *>(event->source());     //need to get the source of the drag, don't know if it's ok to cast as QWidget. when I did it with KListWidgets I casted as KListWidgets and it was ok but now I'm not dragging from a widget of the same class
    if (source && source != this)
    {
        event->setDropAction(Qt::CopyAction);
        event->accept();
    }
    else if (source && source == this)
    {
        event->setDropAction(Qt::MoveAction);
        event->accept();
    }
}

void
FilenameLayoutWidget::dragMoveEvent(QDragMoveEvent *event)          //need to override QListWidget
{
    QWidget *source = qobject_cast<QWidget *>(event->source());     //same as in dragEnterEvent
    if (source && source != this)
    {
        event->setDropAction(Qt::CopyAction);
        event->accept();
    }
    else if (source && source == this)
    {
        event->setDropAction(Qt::MoveAction);
        event->accept();
    }
}

void FilenameLayoutWidget::dropEvent(QDropEvent *event)
{
    QWidget *source = qobject_cast<QWidget *>(event->source());     //not sure how to handle this
    if (source && source != this) {
        //TODO: transfer the string somehow. It was like this when dragging from KListWidget to KListWidget:
        //addItem(event->mimeData()->text());
        addToken();
        event->setDropAction(Qt::CopyAction);
        event->accept();
    }
}



//starts implementation of Token : QLabel

Token::Token(QWidget *parent) : QLabel(parent)
{
    this->setText("TOKEN");
}

void
Token::mouseMoveEvent(QMouseEvent *event)
{
    emit signalMouseMoveEvent(event);           //this with the signals that I'm doing, does it make any sense at all?
}

void
Token::mousePressEvent(QMouseEvent *event)
{
    emit signalMousePressEvent(event);
}
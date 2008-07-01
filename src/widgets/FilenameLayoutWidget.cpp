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

FilenameLayoutWidget::FilenameLayoutWidget(QWidget *parent) : QFrame(parent)
{
    setAcceptDrops(true);
    layout = new QHBoxLayout;
    layout->setSpacing(1);
    setLayout(layout);
    backText = new QLabel;
    backText->setText("<i>Drag tokens here to define a filename scheme.</i>");
    layout->addWidget(backText);
    tokenCount = 0;     //how many tokens have I built, need this to assign unique IDs
}

void
FilenameLayoutWidget::slotAddToken()
{
    this->addToken("TOKEN");
}

void
FilenameLayoutWidget::addToken(QString text){
    if(backText->isVisible())
    {
        backText->hide();
    }

    tokenCount++;
    Token *token = new Token(text + QString::number(tokenCount), this);

    layout->addWidget(token);
    token->show();
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
        //needs to be     x-amarok-tag-token
        debug() << "I'm dragging shit from the token pool";
        addToken(tr("TOKEN"));
        event->setDropAction(Qt::CopyAction);
        event->accept();
    }
}

unsigned int FilenameLayoutWidget::getTokenCount()
{
    return tokenCount;
}



//starts implementation of Token : QLabel

Token::Token(const QString &text, QWidget *parent) : QLabel(parent)
{
    myCount = qobject_cast<FilenameLayoutWidget *>(parent)->getTokenCount();
    QFontMetrics metric(font());
    QSize size = metric.size(Qt::TextSingleLine, text);

    QImage image(size.width() + 12, size.height() + 12, QImage::Format_ARGB32_Premultiplied);
    image.fill(qRgba(0,0,0,0));

    QFont font;
    font.setStyleStrategy(QFont::ForceOutline);

    QPainter painter;
    painter.begin(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(Qt::white);
    painter.drawRoundedRect(QRectF(0.5, 0.5, image.width()-1, image.height()-1), 25, 25, Qt::RelativeSize);
    painter.setFont(font);
    painter.setBrush(Qt::black);
    painter.drawText(QRect(QPoint(6, 6), size), Qt::AlignCenter, text);
    painter.end();
    setPixmap(QPixmap::fromImage(image));
    labelText = text;
}

void
Token::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        int distance = (event->pos() - startPos).manhattanLength();
        if (distance >= KApplication::startDragDistance())
        {
            performDrag(event);
        }
    } 
}

void
Token::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        startPos = event->pos();
}

void
Token::performDrag(QMouseEvent *event)
{
    //transfer of QByteData, not text --thank you Fridge Magnet example from Qt doc
    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    dataStream << labelText << QPoint(event->pos() - rect().topLeft());
    QMimeData *mimeData = new QMimeData;
    mimeData->setData("application/x-amarok-tag-token", itemData);
    mimeData->setText(labelText);
    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setHotSpot(event->pos() - rect().topLeft());
    drag->setPixmap(*pixmap());

    hide();

    if(drag->exec(Qt::MoveAction | Qt::CopyAction, Qt::CopyAction) == Qt::MoveAction)
        close();
    else
        show();
}
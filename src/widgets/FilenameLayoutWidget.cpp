/******************************************************************************
 * Copyright (C) 2008 Teo Mrnjavac <teo.mrnjavac@gmail.com>                   *
 *           (C) 2008 Seb Ruiz <ruiz@kde.org>                                 *
 *           (C) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>          *
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

#include <QMouseEvent>
#include <QByteArray>
#include <QDataStream>
#include <QDragMoveEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QPainter>


FilenameLayoutWidget::FilenameLayoutWidget( QWidget *parent )
    : QFrame( parent )
    , m_tokenCount( 0 )   //how many tokens have I built, need this to assign unique IDs
    , m_parsableScheme( QString()  )
    , m_tokenFactory( new TokenFactory() )
{
    setAcceptDrops( true );
    m_layout = new QHBoxLayout;
    m_layout->setSpacing( 0 );    //this should be coherent when using separators
    setLayout( m_layout );
    
    m_infoText = QString( i18n( "Drag tokens here to define a filename scheme." ) );
    repaint();  //update m_infoText
    
    m_layout->setContentsMargins( 1, 1, 1, 1 );
}

FilenameLayoutWidget::~ FilenameLayoutWidget()
{
    delete m_tokenFactory;
}

// Adds a token with caption text at the index-th place in the 
// FilenameLayoutWidget bar and computes the parsable scheme 
// currently defined by the FilenameLayoutWidget.
void
FilenameLayoutWidget::addToken( Token *token, int index )   //SLOT
{
    if( !token )
        return;

    m_tokenCount++;
    m_layout->insertWidget( index, token );
    
    token->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    token->show();
    
    emit layoutChanged();
}

//Executed whenever a drag object enters the FilenameLayoutWidget
//overrides QListWidget's implementation. this is when the drag becomes droppable
void
FilenameLayoutWidget::dragEnterEvent( QDragEnterEvent *event )
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

//Executed whenever a drag object moves inside the FilenameLayoutWidget
void
FilenameLayoutWidget::dragMoveEvent( QDragMoveEvent *event )
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

//Handles the insertion of a token over another depending on its position, computes the index and calls addToken
void
        FilenameLayoutWidget::insertOverChild( Token *childUnder, Token *token, QDropEvent *event )
{
    if ( !childUnder )
        return;
    
    int index = m_layout->indexOf( childUnder );

    if( event->pos().x() < childUnder->pos().x() + childUnder->size().width() / 2 )
        addToken( token, index );
    else
        addToken( token, index + 1 );
}

//Executed whenever a valid drag object is dropped on the FilenameLayoutWidget. Will call addToken and insertOverChild.
void
FilenameLayoutWidget::dropEvent( QDropEvent *event )
{
    QWidget *source = qobject_cast<QWidget *>( event->source() );     //not sure how to handle this
    QByteArray itemData = event->mimeData()->data( "application/x-amarok-tag-token" );
    QDataStream dataStream(&itemData, QIODevice::ReadOnly);
    
    QString tokenName;
    QString tokenIconName;
    int tokenValue;
    dataStream >> tokenName;
    dataStream >> tokenIconName;
    dataStream >> tokenValue;

    Token * droppedtoken = m_tokenFactory->createToken( tokenName, tokenIconName, tokenValue );

    if ( source && source != this )
        event->setDropAction( Qt::CopyAction );
    else if ( source && source == this )
        event->setDropAction( Qt::MoveAction );
    
    Token *childUnder = qobject_cast< Token * >( childAt( event->pos() ) );
    // if not dropping on an existing token
    if( childUnder == 0 )
    {
        // if the bar is empty
        if( !m_tokenCount )
            addToken( droppedtoken );
        //if the bar is not empty and I'm still not dropping on an existing token
        else
        {
            QPoint fixedPos = QPoint( event->pos().x(), size().height() / 2 );      //first I lower the y coordinate of the drop, this should handle the drops higher and lower than the tokens
            childUnder = qobject_cast< Token * >( childAt( fixedPos ) );            //and I look for a child (token) on these new coordinates
            if( !childUnder )                                                       //if there's none, then I'm either at the beginning or at the end of the bar
            {
                if( fixedPos.x() < childrenRect().topLeft().x() )                   //if I'm dropping before all the tokens
                {
                    fixedPos = QPoint( fixedPos.x() + 10, fixedPos.y() );
                }
                else if( fixedPos.x() > childrenRect().topLeft().x() + childrenRect().size().width() )          //this covers if I'm dropping after all the tokens or in between
                {
                    fixedPos = QPoint( fixedPos.x() - 10, fixedPos.y() );           //to self: why the f am I moving to the left as else? I should do that only if I'm on the end of the childrenRect
                }
                childUnder = qobject_cast< Token * >( childAt( fixedPos ) );
                QWidget * fakeChild = childAt( fixedPos );
                if( childUnder == 0)
                {
                    if( fakeChild == 0 )
                    {
                        debug()<< "There's really no QWidget under the fixedPos";
                    }
                    else
                    {
                        debug()<<"There's something under fixedPos but it's not a Token";
                        childUnder = qobject_cast< Token * >( childAt( fixedPos )->parent() );
                    }
                }
                
                if( !childUnder )
                    error() << "ERROR: childUnder is null";
                insertOverChild( childUnder, droppedtoken, event );
            }
            else // token found
            {
                insertOverChild( childUnder, droppedtoken, event );
            }
        }
    }
    else // Dropping on an existing token, that's easy
    {
        insertOverChild( childUnder, droppedtoken, event );
    }
    event->accept();
    
}

//Access for m_tokenCount
unsigned int
FilenameLayoutWidget::getTokenCount() const
{
    return m_tokenCount;
}

//Executed whenever the mouse moves over the FilenameLayoutWidget. Needed for computing the start condition of the drag.
void
FilenameLayoutWidget::mouseMoveEvent( QMouseEvent *event )
{
    if ( event->buttons() & Qt::LeftButton )
    {
        int distance = ( event->pos() - m_startPos ).manhattanLength();
        if ( distance >= KApplication::startDragDistance() ) // TODO: maybe it's not my business to say it but this should be done in PlaylistView too.
        {
            performDrag( event );
        }
    }
}

//Executed whenever the mouse is pressed over the FilenameLayoutWidget. Needed for computing the start condition of the drag.
void
FilenameLayoutWidget::mousePressEvent( QMouseEvent *event )
{
    if ( event->button() == Qt::LeftButton )
        m_startPos = event->pos();
    if ( event->button() == Qt::MidButton )
    {
        if( childAt( event->pos() ) == 0)
            return;
        Token *child = qobject_cast< Token * >( childAt( event->pos() ) );
        if( !child )
        {
            child = qobject_cast< Token * >( childAt( event->pos() )->parent() );
            if( !child )
                return;
        }
        delete child;
        m_tokenCount--;
        
        repaint();  //update m_infoText

        emit layoutChanged();
    }
}

//Executed when a drag is initiated from the FilenameLayoutWidget. If valid, grabs a token and instances a QDrag for it. Calls generateParsableScheme() when done.
void
FilenameLayoutWidget::performDrag( QMouseEvent *event )
{
    // transfer of QByteData, not text
    Token *child = qobject_cast<Token*>( childAt( event->pos() ) );
    if ( !child )
        return;
    
    QByteArray itemData;
    
    QDataStream dataStream( &itemData, QIODevice::WriteOnly );
    dataStream << child->name() << child->iconName() << child->value();

    QMimeData *mimeData = new QMimeData();
    mimeData->setData( "application/x-amarok-tag-token", itemData );
    
    QDrag *drag =  new QDrag( this );
    drag->setMimeData( mimeData );
    drag->setHotSpot( event->pos() - child->rect().topLeft() - child->pos() );        //I grab the initial position of the item I'm dragging
    
    drag->setPixmap( QPixmap::grabWidget( child ) );       //need to get pixmap from widget

    delete child;
    m_tokenCount--;
    repaint();  //update m_infoText
    
    drag->exec(Qt::MoveAction | Qt::CopyAction, Qt:: CopyAction);
    emit layoutChanged();
}


void
FilenameLayoutWidget::removeAllTokens()
{
    QLayoutItem *child; //Qt docs suggest this for safe deletion of all the elements of a QLayout.
    while( ( child = m_layout->takeAt(0) ) )
    {
        delete child->widget();
        delete child;
    }
    m_tokenCount = 0;
    repaint();  //update m_infoText
    
    emit layoutChanged();
}

void
FilenameLayoutWidget::paintEvent( QPaintEvent *event )
{
    if( !m_tokenCount )
    {
        QPainter p(this);
        QFontMetrics fm( p.font() );
        p.drawText( QRect( rect().topLeft() + QPoint( 4, 4 ), rect().size() - QSize( 8, 8 ) ),
                    Qt::AlignCenter, fm.elidedText( m_infoText, Qt::ElideRight, rect().width() - 8 ) );
    }
    QFrame::paintEvent( event );
}

QList< Token *> FilenameLayoutWidget::currentTokenLayout()
{
    QList< Token *> list;
    for( int i = 0; i < m_layout->count(); ++i)
    {
        // getting a Token by grabbing a QLayoutItem* at index i and grabbing his QWidget.
        Token *token = qobject_cast<Token*>( m_layout->itemAt(i)->widget() );
        if( !token )
            continue;

        list << token;
    }

    return list;
}

void FilenameLayoutWidget::setCustomTokenFactory( TokenFactory * factory )
{
    delete m_tokenFactory;
    m_tokenFactory = factory;
}




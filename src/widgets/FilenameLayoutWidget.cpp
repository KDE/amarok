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


FilenameLayoutWidget::FilenameLayoutWidget( QWidget *parent )
    : QFrame( parent )
    , m_tokenCount( 0 )   //how many tokens have I built, need this to assign unique IDs
    , m_parsableScheme( "" )
{
    setAcceptDrops( true );
    layout = new QHBoxLayout;
    layout->setSpacing( 0 );    //this should be coherent when using separators
    setLayout( layout );
    backText = new QLabel;
    backText->setText( i18n( "<div align=center><i>Drag tokens here to define a filename scheme.</i></div>" ) );
    layout->addWidget( backText );
    tokenList = new QList< Token * >();
}

//Adds a token with caption text at the index-th place in the FilenameLayoutWidget bar and computes the parsable scheme currently defined by the FilenameLayoutWidget.
void
FilenameLayoutWidget::addToken( QString text, int index )   //SLOT
{
    if( !m_tokenCount )
    {
        backText->hide();
    }

    m_tokenCount++;
    Token *token = new Token( text, this );

    debug()<< "I am adding a token, the index I got is " << index << ".";
    if( index == 0)
    {
        layout->addWidget( token );
        tokenList->append( token );
        debug() << "\t\ttokenList action: append";
    }
    else
    {
        layout->insertWidget( index, token );
        tokenList->insert( index - 1, token );
        debug()<< "\t\ttokenList action: inserted at " << index - 1 << ".";
    }
    
    token->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    token->show();

    if( token->text() == "<space>" )
    {
        token->setText( " " );
    }

    //testing, remove when done
    //token->setText( token->text() + " " + QString::number( layout->indexOf( token ) ) );
    //end testing block

    //debug stuff follows
    foreach(Token *temp, *tokenList)
    {
        debug() << tokenList->indexOf( temp ) << " .......... " << layout->indexOf( temp ) << " .......... " << temp->text();
    }
    generateParsableScheme();
    emit schemeChanged();
}

//Executed whenever a drag object enters the FilenameLayoutWidget
void
FilenameLayoutWidget::dragEnterEvent( QDragEnterEvent *event )        //overrides QListWidget's implementation. this is when the drag becomes droppable
{
    QWidget *source = qobject_cast<QWidget *>( event->source() );
    if ( source && source != this )
    {
        event->setDropAction( Qt::CopyAction );
        event->accept();
        debug() << "dragEnterEvent, source is != this";
    }
    else if ( source && source == this )
    {
        event->setDropAction( Qt::MoveAction );
        event->accept();
        debug() << "dragEnterEvent, source is == this";
    }
}

//Executed whenever a drag object moves inside the FilenameLayoutWidget
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

//Handles the insertion of a token over another depending on its position, computes the index and calls addToken
void
FilenameLayoutWidget::insertOverChild( Token *childUnder, QString &textFromMimeData, QDropEvent *event )
{
    int index = layout->indexOf( childUnder );
    if( event->pos().x() < childUnder->pos().x() + childUnder->size().width() / 2 )
    {
        addToken( textFromMimeData, index );
    }
    else
    {
        addToken( textFromMimeData, index + 1 );
    }
}

//Executed whenever a valid drag object is dropped on the FilenameLayoutWidget. Will call addToken and insertOverChild.
void
FilenameLayoutWidget::dropEvent( QDropEvent *event )
{
    QWidget *source = qobject_cast<QWidget *>( event->source() );     //not sure how to handle this
    QByteArray itemData = event->mimeData()->data( "application/x-amarok-tag-token" );
    QDataStream dataStream(&itemData, QIODevice::ReadOnly);
    QString textFromMimeData;
    dataStream >> textFromMimeData;
    if ( source && source != this )
    {
        debug() << "I am dragging from the token pool";
        event->setDropAction( Qt::CopyAction );
    }
    else if ( source && source == this )
    {
        debug() << "I am dragging from the layout widget";
        event->setDropAction( Qt::MoveAction );
    }

    Token *childUnder = qobject_cast< Token * >( childAt( event->pos() ) );
    if( childUnder == 0 )   //if I'm not dropping on an existing token
    {
        if( !m_tokenCount )   //if the bar is empty
        {
            addToken( textFromMimeData );
        }
        else                //if the bar is not empty and I'm still not dropping on an existing token
        {
            QPoint fixedPos = QPoint( event->pos().x(), size().height() / 2 );      //first I lower the y coordinate of the drop, this should handle the drops higher and lower than the tokens
            childUnder = qobject_cast< Token * >( childAt( fixedPos ) );            //and I look for a child (token) on these new coordinates
            if( childUnder == 0 )                                                   //if there's none, then I'm either at the beginning or at the end of the bar
            {
                if( fixedPos.x() < childrenRect().topLeft().x() )                   //if I'm dropping before all the tokens
                {
                    fixedPos = QPoint( fixedPos.x() + 10, fixedPos.y() );
                }
                else                                                                //this covers if I'm dropping after all the tokens or in between
                {
                    fixedPos = QPoint( fixedPos.x() - 10, fixedPos.y() );
                }
                childUnder = qobject_cast< Token * >( childAt( fixedPos ) );
                insertOverChild( childUnder, textFromMimeData, event );
            }
            else                                                                    //if I find a token, I'm done
            {
                insertOverChild( childUnder, textFromMimeData, event );
            }
        }
    }
    else                    //I'm dropping on an existing token, that's easy
    {
        insertOverChild( childUnder, textFromMimeData, event );
    }
    event->accept();
    debug() << "Accepted drop event";
    
}

//Access for m_tokenCount
unsigned int
FilenameLayoutWidget::getTokenCount()
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
        if ( distance >= KApplication::startDragDistance() )    //TODO:maybe it's not my business to say it but this should be done in PlaylistView too.
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
}

//Executed when a drag is initiated from the FilenameLayoutWidget. If valid, grabs a token and instances a QDrag for it. Calls generateParsableScheme() when done.
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
    QDrag *drag = new QDrag( this );
    drag->setMimeData( mimeData );
    drag->setHotSpot( event->pos() - child->rect().topLeft() - child->pos() );        //I grab the initial position of the item I'm dragging
    
    drag->setPixmap( QPixmap::grabWidget( child ) );       //need to get pixmap from widget

    //child->close();
    tokenList->removeAt( layout->indexOf( child ) - 1 );
    debug() << "Deleting at " << layout->indexOf( child ) - 1;
    delete child;
    m_tokenCount--;

    if( !m_tokenCount )
    {
        backText->show();
    }
    
    drag->exec(Qt::MoveAction | Qt::CopyAction, Qt:: CopyAction);
    generateParsableScheme();
    emit schemeChanged();
}

//Iterates over the elements of the FilenameLayoutWidget bar (really over the elements of a QList that stores the indexes of the tokens) and generates a string that TagGuesser can digest.
void
FilenameLayoutWidget::generateParsableScheme()      //invoked on every change of the layout
{
    //with m_parsableScheme
    m_parsableScheme = "";
    foreach( Token *token, *tokenList)
    {
        QString current = token->text();
        if( current == i18n( "Track" ) )
        {
            m_parsableScheme += "%track";
        }
        else if( current == i18n( "Title" ) )
        {
            m_parsableScheme += "%title";
        }
        else if( current == i18n( "Artist" ) )
        {
            m_parsableScheme += "%artist";
        }
        else if( current == i18n( "Composer" ) )
        {
            m_parsableScheme += "%composer";
        }
        else if( current == i18n( "Year" ) )
        {
            m_parsableScheme += "%year";
        }
        else if( current == i18n( "Album" ) )
        {
            m_parsableScheme += "%album";
        }
        else if( current == i18n( "Comment" ) )
        {
            m_parsableScheme += "%comment";
        }
        else if( current == i18n( "Genre" ) )
        {
            m_parsableScheme += "%genre";
        }
        else
        {
            m_parsableScheme += current;
        }
    }
    debug() << "||| PARSABLE SCHEME ||| >>  " << m_parsableScheme;
}

//Access for m_parsableScheme.
QString
FilenameLayoutWidget::getParsableScheme()
{
    return m_parsableScheme;
}




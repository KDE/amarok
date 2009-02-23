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
#include "DragStack.h"

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
{
    m_dragstack = new DragStack( "application/x-amarok-tag-token", this );
    m_dragstack->setRowLimit( 1 );
    m_dragstack->setContentsMargins( 1, 1, 1, 1 );
    connect ( m_dragstack, SIGNAL( changed() ), this, SLOT( update() ) );
    connect ( m_dragstack, SIGNAL( changed() ), this, SIGNAL( layoutChanged() ) );

    m_infoText = QString( i18n( "Drag tokens here to define a filename scheme." ) );

    repaint();  //update m_infoText
}

// Adds a token with caption text at the index-th place in the 
// FilenameLayoutWidget bar and computes the parsable scheme 
// currently defined by the FilenameLayoutWidget.
void
FilenameLayoutWidget::addToken( Token *token, int index )   //SLOT
{
    m_dragstack->insertToken( token, 0, index );
    // why?
    token->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
}


//Access for m_tokenCount
unsigned int
FilenameLayoutWidget::getTokenCount() const
{
    return m_dragstack->count( 0 );
}


void
FilenameLayoutWidget::removeAllTokens()
{
    m_dragstack->clear();
}

void
FilenameLayoutWidget::paintEvent( QPaintEvent *event )
{
    if( !m_dragstack->count() )
    {
        QPainter p(this);
        p.drawText( rect().adjusted( 4, 4, -4, -4 ), Qt::AlignCenter,
                    QFontMetrics( p.font() ).elidedText( m_infoText, Qt::ElideRight, rect().width() - 8 ) );
        // depending on the compiler this can be necessary, as Qt doesn't allow opening two painters
        // on the same widget and some compilers don't delete stack items directly on conditional statement end
        p.end();
    }
    QFrame::paintEvent( event );
}

QList< Token *> FilenameLayoutWidget::currentTokenLayout()
{
    return m_dragstack->drags( 0 );
}

void FilenameLayoutWidget::setCustomTokenFactory( TokenFactory * factory )
{
    m_dragstack->setCustomTokenFactory( factory );
}

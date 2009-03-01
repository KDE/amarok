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

#include "TokenLayoutWidget.h"
#include "Debug.h"
#include "TokenDropTarget.h"

#include <KApplication>

#include <QMouseEvent>
#include <QByteArray>
#include <QDataStream>
#include <QDragMoveEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QPainter>


TokenLayoutWidget::TokenLayoutWidget( QWidget *parent )
    : QFrame( parent )
{
    QHBoxLayout *layout = new QHBoxLayout();
    layout->setContentsMargins( 0, 0, 0, 0 );

    m_dropTarget = new TokenDropTarget( "application/x-amarok-tag-token", this );
    m_dropTarget->setRowLimit( 1 );
    m_dropTarget->layout()->setContentsMargins( 1, 1, 1, 1 );
    connect ( m_dropTarget, SIGNAL( changed() ), this, SLOT( update() ) );
    connect ( m_dropTarget, SIGNAL( changed() ), this, SIGNAL( layoutChanged() ) );

    m_infoText = QString( i18n( "Drag tokens here to define a filename scheme." ) );

    layout->addWidget( m_dropTarget );
    setLayout( layout );

    repaint();  //update m_infoText
}

// Adds a token with caption text at the index-th place in the 
// TokenLayoutWidget bar and computes the parsable scheme
// currently defined by the TokenLayoutWidget.
void
TokenLayoutWidget::addToken( Token *token, int index )   //SLOT
{
    m_dropTarget->insertToken( token, 0, index );
    // why?
    token->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
}


//Access for m_tokenCount
unsigned int
TokenLayoutWidget::getTokenCount() const
{
    return m_dropTarget->count( 0 );
}


void
TokenLayoutWidget::removeAllTokens()
{
    m_dropTarget->clear();
}

void
TokenLayoutWidget::paintEvent( QPaintEvent *event )
{
    if( !m_dropTarget->count() )
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

QList< Token *> TokenLayoutWidget::currentTokenLayout()
{
    return m_dropTarget->drags( 0 );
}

void TokenLayoutWidget::setCustomTokenFactory( TokenFactory * factory )
{
    m_dropTarget->setCustomTokenFactory( factory );
}

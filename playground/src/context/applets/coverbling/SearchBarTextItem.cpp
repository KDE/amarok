/****************************************************************************************
 * Copyright (c) 2010 Emmanuel Wagner <manu.wagner@sfr.fr>                              *                          
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


#include "SearchBarTextItem.h"
#include <QKeyEvent>
#include <QTextDocument>

SearchBarTextItem::SearchBarTextItem( QGraphicsItem* parent, QGraphicsScene* scene )

        : QGraphicsTextItem( parent, scene )
{
    QTextDocument* textDocument = document();//get document
    if ( textDocument )
        textDocument->setMaximumBlockCount( 1 ); //i set max block count to 5 e.g.
}

void SearchBarTextItem::keyPressEvent( QKeyEvent* Event )

{
    if ( Event )
    {
        if ( Event->key() == Qt::Key_Return || Event->key() == Qt::Key_Enter )
        {
            emit editionValidated( toPlainText() );
        }
    }
    QGraphicsTextItem::keyPressEvent( Event );
}
void SearchBarTextItem::mousePressEvent ( QGraphicsSceneMouseEvent * event )
{
	setPlainText( "" );
	QGraphicsTextItem::mousePressEvent( event );
} 

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

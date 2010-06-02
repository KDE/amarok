#include "MyGraphicsTextItem.h"
#include <QKeyEvent>
#include <QTextDocument>

MyGraphicsTextItem::MyGraphicsTextItem( QGraphicsItem* parent, QGraphicsScene* scene )

        : QGraphicsTextItem( parent, scene )
{
    QTextDocument* textDocument = document();//get document
    if ( textDocument )
        textDocument->setMaximumBlockCount( 1 ); //i set max block count to 5 e.g.
}

void MyGraphicsTextItem::keyPressEvent( QKeyEvent* Event )

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
void MyGraphicsTextItem::mousePressEvent ( QGraphicsSceneMouseEvent * event )
{
	setPlainText( "" );
	QGraphicsTextItem::mousePressEvent( event );
} 

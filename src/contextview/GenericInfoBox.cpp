/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "GenericInfoBox.h"
#include "amarok.h"

#include <QGraphicsTextItem>

using namespace Context;

GenericInfoBox::GenericInfoBox( QGraphicsItem* parent, QGraphicsScene *scene ) : ContextBox( parent, scene )  {}

void GenericInfoBox::setContents( const QString& html )
{
    m_content = new QGraphicsTextItem( m_contentRect );
    m_content->setHtml( html);
    init();
	
}

void GenericInfoBox::init()
{
    m_content->setTextWidth( m_contentRect->rect().width() );// we don't want it to be super-wide.
    m_content->setTextInteractionFlags( Qt::TextSelectableByKeyboard |
                                        Qt::TextSelectableByMouse    |
                                        Qt::LinksAccessibleByMouse   |
                                        Qt::LinksAccessibleByKeyboard);
    connect( m_content, SIGNAL( linkActivated ( QString ) ), this, SLOT( externalUrl( QString ) ) ); // make urls work
    int width =  (int) m_content->boundingRect().width();
    int height = (int) m_content->boundingRect().height();
    //setContentRectSize( QSize( width, height ) );
}

void GenericInfoBox::clearContents() {
    delete m_content;
    /*m_content = new QGraphicsTextItem( m_contentRect );
    init();*/
}

void GenericInfoBox::externalUrl( const QString& url )
{
    Amarok::invokeBrowser( url );
}
#include "GenericInfoBox.moc"

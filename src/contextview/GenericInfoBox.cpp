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

#include <QGraphicsTextItem>

using namespace Context;

GenericInfoBox::GenericInfoBox( QGraphicsItem* parent, QGraphicsScene *scene ) : ContextBox( parent, scene ) {}

void GenericInfoBox::setContents( const QString& html )
{
	QGraphicsTextItem* content = new QGraphicsTextItem( m_contentRect );
	content->setHtml( html);
	
}

#include "GenericInfoBox.moc"

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

#include "ContextScene.h"

#include "amarok.h"
#include "amarokconfig.h"
#include "debug.h"

#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsView>
#include <QMimeData>

#include <KMimeType>

// NOTE for now ContextScene is a completely useless class (performs the same as
// Corona. but i think i might want to add stuff to it soon.

namespace Context
{

ContextScene::ContextScene(QObject * parent)
    : Plasma::Corona( parent )
{
}

ContextScene::ContextScene(const QRectF & sceneRect, QObject * parent )
    : Plasma::Corona( sceneRect, parent )
{
}

ContextScene::ContextScene(qreal x, qreal y, qreal width, qreal height, QObject * parent)
    : Plasma::Corona( x, y, width, height, parent )
{
}

ContextScene::~ContextScene()
{
}

void ContextScene::appletDestroyed(QObject* object)
{
    DEBUG_BLOCK
    Corona::appletDestroyed( object ); // corona needs to clean up internally
    emit appletRemoved( object );
}

} // Context namespace

#include "ContextScene.moc"

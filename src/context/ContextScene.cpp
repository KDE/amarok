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

#include "Amarok.h"
#include "amarokconfig.h"
#include "Debug.h"
#include "plasma/containment.h"
#include "plasma/theme.h"

#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsView>
#include <QMimeData>

#include <KMimeType>

// NOTE for now ContextScene is a completely useless class (performs the same as
// Corona. but i think i might want to add stuff to it soon.

namespace Context
{

ContextScene::ContextScene( QObject * parent )
    : Plasma::Corona( parent )
{
    setBackgroundBrush( Qt::NoBrush );
}

ContextScene::~ContextScene()
{
    DEBUG_BLOCK
}

void ContextScene::loadDefaultSetup()
{
    Plasma::Containment* c = addContainment( "context" );
    c->setScreen( 0 );
    c->setFormFactor( Plasma::Planar );
}

void ContextScene::dragMoveEvent( QGraphicsSceneDragDropEvent * event )
{
    DEBUG_BLOCK
    Q_UNUSED( event );
}

void
ContextScene::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
    DEBUG_BLOCK
    debug() << "event->pos(): " << event->pos();
    if( itemAt( event->pos() ) )
    {
        debug() << "YES!!!";
    }
    else
    {
        debug() << ":(";
    }
    Plasma::Corona::mousePressEvent( event );
}

// TODO port to Containment
/*
void ContextScene::appletDestroyed(QObject* object)
{
    DEBUG_BLOCK
    Corona::appletDestroyed( object ); // corona needs to clean up internally
    emit appletRemoved( object );
}*/

} // Context namespace

#include "ContextScene.moc"

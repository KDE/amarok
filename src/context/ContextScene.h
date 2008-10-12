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

#ifndef AMAROK_CONTEXT_SCENE_H
#define AMAROK_CONTEXT_SCENE_H

#include "amarok_export.h"
#include "Applet.h"
#include "Context.h"
#include "plasma/corona.h"

#include <QGraphicsSceneMouseEvent>

namespace Context
{

class AMAROK_EXPORT ContextScene : public Plasma::Corona
{
    Q_OBJECT
public:
    explicit ContextScene(QObject * parent = 0);
    ~ContextScene();

    void loadDefaultSetup();

signals:
    void appletRemoved( QObject *object );

protected slots:
    // TODO: Port to Containment
//     void appletDestroyed(QObject* object);

protected:
    void dragMoveEvent( QGraphicsSceneDragDropEvent * event );
};

} // Context namespace

#endif

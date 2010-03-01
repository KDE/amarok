/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
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

#ifndef AMAROK_CONTEXT_SCENE_H
#define AMAROK_CONTEXT_SCENE_H

#include "amarok_export.h"
#include "Applet.h"
#include "Context.h"

#include <plasma/corona.h>

#include <QGraphicsSceneMouseEvent>

namespace Context
{

/**
 * The ContextScene is a very simple QGraphicsScene, it does the same thing as a Plasma::Corona.
 * The only bit that is important is that it controls what the default containtainment to be loaded should be.
 */
class AMAROK_EXPORT ContextScene : public Plasma::Corona
{
    Q_OBJECT
public:
    explicit ContextScene(QObject * parent = 0);
    ~ContextScene();

    void loadDefaultSetup();

signals:
    void appletRemoved( QObject *object );

};

} // Context namespace

#endif

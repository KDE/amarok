

/***************************************************************************
*   copyright: Max Howell <max.howell@methylblue.com>, (C) 2004            *
*              Dan Meltzer <parallelgrapefruit@gmail.com>, (C) 2007      *
***************************************************************************/


/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROKTOOLBAR_H
#define AMAROKTOOLBAR_H

#include "ActionClasses.h"
#include "EngineController.h"

#include <ktoolbar.h>

namespace Amarok {

    class ToolBar : public KToolBar
    {
        public:
            /**
            * Create a Toolbar with no Border.
            * @param parent The Main Window that
            * this toolbar belongs to.
            * @param name The QObject name of this toolbar
            */
            ToolBar( QMainWindow *parent, const char *name )
                : KToolBar( name, parent, Qt::TopToolBarArea, true, false, false )
            {
            }

            /**
             * Create a borderless toolbar that can live anywhere.
             * @param parent The Widget that should be the parent of this toolbar
             * @param name The QObject name of this toolbar
             */
            ToolBar( QWidget *parent )
                : KToolBar( parent, false, false )
            {
            }

        protected:
            virtual void paintEvent( QPaintEvent * )
            {
            }

    };

}

#endif

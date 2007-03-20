/***************************************************************************
*   copyright: Max Howell <max.howell@methylblue.com>, (C) 2004            *
*              Dan Meltzer <hydrogen@notyetimplemented.com>, (C) 2007      *
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

#include "actionclasses.h"
#include "enginecontroller.h"

#include <ktoolbar.h>

namespace Amarok {

    class ToolBar : public KToolBar
    {
        public:
            /**
            * Create a Toolbar with no Border.
            * @param parent The Widget that should be the parent of this toolbar
            * @param name The QObject name of this toolbar
            */
            ToolBar( QMainWindow *parent, const char *name )
                : KToolBar( name, parent, Qt::TopToolBarArea, true, false, false )
            {
            }

            ToolBar( QWidget *parent )
                : KToolBar( parent, false, false )
            {
            }

        protected:
            virtual void paintEvent( QPaintEvent * )
            {
            }

    };

    class PrettyToolBar : public ToolBar
    {
        public:
            /**
            * Create a Toolbar with a nice context menu that increases
            * volume on wheel events.
            * @param parent The Widget that should be the parent of this toolbar
            * @param name The QObject name of this toolbar
            */
            PrettyToolBar( QWidget *parent, const char *name )
                : ToolBar( parent )
            {
            }

        protected:
            virtual void
            contextMenuEvent( QContextMenuEvent *e ) {
                Amarok::Menu::instance()->popup( e->globalPos() );
            }

            virtual void
            wheelEvent( QWheelEvent *e ) {
                EngineController::instance()->increaseVolume( e->delta() / Amarok::VOLUME_SENSITIVITY );
            }
    };

}

#endif

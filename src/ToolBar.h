/****************************************************************************************
 * Copyright (c) 2004 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2007 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#ifndef AMAROKTOOLBAR_H
#define AMAROKTOOLBAR_H

#include "ActionClasses.h"
#include "EngineController.h"
#include "PaletteHandler.h"

#include <ktoolbar.h>

#include <QPainter>
#include <QPaintEvent>
#include <QPalette>

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

    class ColoredToolBar : public KToolBar
    {
        public:
         /**
         * Create a colored Toolbar.
         * @param parent The Main Window that
         * this toolbar belongs to.
         * @param name The QObject name of this toolbar
             */
            ColoredToolBar( QMainWindow *parent, const char *name )
            : KToolBar( name, parent, Qt::TopToolBarArea, true, false, false )
            {
            }

            /**
             * Create a colored toolbar that can live anywhere.
             * @param parent The Widget that should be the parent of this toolbar
             * @param name The QObject name of this toolbar
             */
            ColoredToolBar( QWidget *parent )
            : KToolBar( parent, false, false )
            {
            }


        protected:
            virtual void paintEvent( QPaintEvent * event )
            {
                QPainter painter( this );

                painter.setRenderHint( QPainter::Antialiasing );
                painter.save();

                QColor col = PaletteHandler::highlightColor();
                qreal radius = 6;

                QPainterPath outline;
                outline.addRoundedRect( event->rect(), radius, radius );
                painter.fillPath( outline, QBrush( col ) );

                painter.restore();
            }
    };

}

#endif

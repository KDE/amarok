/***************************************************************************
 *   Copyright (C) 2004, 2005 Max Howell <max.howell@methylblue.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef BROWSER_TOOLBAR_H
#define BROWSER_TOOLBAR_H

#include <ktoolbar.h>

namespace Browser
{
    class ToolBar : public KToolBar
    {
        public:
            ToolBar( QWidget *parent )
                    : KToolBar( parent, "NotMainToolBar" )
            {
                setMovingEnabled(false);
                setFlat(true);
                setIconSize( 16 );
                setEnableContextMenu( false );
            }
    };
}

#endif

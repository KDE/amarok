/*******************************************************************************
* copyright              : (C) 2008 William Viana Soares <vianasw@gmail.com>   *
*                                                                              *
********************************************************************************/

/*******************************************************************************
*                                                                              *
*   This program is free software; you can redistribute it and/or modify       *
*   it under the terms of the GNU General Public License as published by       *
*   the Free Software Foundation; either version 2 of the License, or          *
*   (at your option) any later version.                                        *
*                                                                              *
********************************************************************************/

#ifndef TOOLBOX_ICON_H
#define TOOLBOX_ICON_H

#include "amarok_export.h"

#include <plasma/widgets/icon.h>

class QPainterPath;

class AMAROK_EXPORT ToolBoxIcon: public Plasma::Icon
{
    Q_OBJECT
public:
    explicit ToolBoxIcon( QGraphicsItem *parent = 0 );
    
    /**
     * reimplemented from Plasma::Icon
     */
    QPainterPath shape() const;

};

#endif

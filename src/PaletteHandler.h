/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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
 
#ifndef PALETTEHANDLER_H
#define PALETTEHANDLER_H

#include "amarok_export.h"

#include <QObject>
#include <QTreeView>

class PaletteHandler;

namespace The {
    AMAROK_EXPORT PaletteHandler* paletteHandler();
}

/**
A small singleton class to handle propagating palette change notifications and hold some utility functions for updating certain widgets

    @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class AMAROK_EXPORT PaletteHandler : public QObject
{
    Q_OBJECT

friend PaletteHandler* The::paletteHandler();

public:
    ~PaletteHandler();

    QPalette palette();

    void setPalette( const QPalette & palette );
    void updateItemView( QAbstractItemView * view );

    /**
     * Returns the highlight color which should be used instead of the color from KDE.
     * @return Highlight color, which is the KDE highlight color, with reduced saturation (less contrast).
     */
    static QColor highlightColor();  //defined in App.cpp

    /**
     * Returns the highlight color which should be used instead of the color from KDE.
     * @param  Decimal percentage to saturate the highlight color. Will reduce (or magnify) the saturation in HSV representation of the color.
     * @param  Decimal percentage to multiply the value of the HSV color with.
     * @return Highlight color, which is the KDE highlight color, with reduced saturation (less contrast).
     */
    static QColor highlightColor( qreal percentSaturation, qreal percentValue );  //defined in App.cpp

    
signals:
    void newPalette( const QPalette & palette );
    
private:
    PaletteHandler( QObject* parent = 0 );

    QPalette m_palette;
};

#endif

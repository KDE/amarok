/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
#include <QPalette>

class QAbstractItemView;
class PaletteHandler;

namespace The {
    AMAROK_EXPORT PaletteHandler* paletteHandler();
}

/**
A small singleton class to handle propagating palette change notifications and hold some utility functions for updating certain widgets

    @author Nikolaj Hald Nielsen <nhn@kde.org>
*/
class AMAROK_EXPORT PaletteHandler : public QObject
{
    Q_OBJECT

friend PaletteHandler* The::paletteHandler();

public:
    ~PaletteHandler();

    QPalette palette() const;

    void setPalette( const QPalette & palette );

    /** Gives the item view a special darker palette and transparent background.
        You need to connect to the newPalette signal afterwards because this
        darker palette does not automatically update.
        @param view the item view.
    */
    void updateItemView( QAbstractItemView * view );

    /**
    * Returns the foreground color for the painter by checking the painting QWidget::foregroundRole() and falling back to
    * QPalette::WindowText (or QPalette::HighlightedText if @param selected)
    * Uses the widgets palette or the application palette as fallback
    */    
    QColor foregroundColor( const QPainter *p, bool selected = false );

    /**
     * Returns the highlight color which should be used instead of the color from KDE.
     * @param percentSaturation Decimal percentage to saturate the highlight color. Will
     *        reduce (or magnify) the saturation in HSV representation of the color.
     *        Defaults to 50%
     * @param percentValue Decimal percentage to multiply the value of the HSV color with.
     *        Defaults to 100%.
     * @return Highlight color, which is the KDE highlight color, with reduced saturation
     *         (less contrast).
     */
    static QColor highlightColor( qreal percentSaturation = 0.5, qreal percentValue = 1.0 );

    /**
     * Returns the background color used in context applets.
     */
    static QColor backgroundColor();

    /**
     * Returns the alternate background color used in context applets.
     */
    static QColor alternateBackgroundColor();

Q_SIGNALS:
    void newPalette( const QPalette & palette );

private:
    PaletteHandler( QObject* parent = 0 );

    QPalette m_palette;
};

#endif

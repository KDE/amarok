/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2008 Jeff Mitchell <kde-dev@emailgoeshere.com>                         *
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
 
#ifndef SVGHANDLER_H
#define SVGHANDLER_H

#include "amarok_export.h"

#include <KPixmapCache>
#include <QReadWriteLock>
#include <KSvgRenderer>

#include <QPixmap>
#include <QString>

class SvgHandler;

namespace The {
    AMAROK_EXPORT SvgHandler* svgHandler();
}

/**
A class to abstract out some common operations of users of tinted svgs
*/
class AMAROK_EXPORT SvgHandler : public QObject
{
    Q_OBJECT

    friend SvgHandler* The::svgHandler();

    public:
        ~SvgHandler();

        KSvgRenderer* getRenderer( const QString &name );
        KSvgRenderer* getRenderer();
        QPixmap renderSvg( const QString &name, const QString& keyname, int width, int height, const QString& element = QString() );

        /**
        * Overloaded function that uses the current theme
        * @param keyname the name of the key to save in the cache
        * @param width Width of the resulting pixmap
        * @param height Height of the resulting pixmap
        * @param element The theme element to render ( if none the entire svg is rendered )
        * @return The svg element/file rendered into a pixmap
        */
        QPixmap renderSvg( const QString& keyname, int width, int height, const QString& element = QString() );
        
        /**
         * Yet another overloaded function. This one renders the svg element and adds half a divider element to the top and the bottom
         * so it looks sane when multiple elements with the same width are stacked.
         *
         * @param keyname the name of the key to save in the cache.
         * @param width Width of the resulting pixmap.
         * @param height Height of the resulting pixmap.
         * @param element The theme element to render ( if none the entire svg is rendered )
         * @return The svg element/file rendered into a pixmap.
         */
        QPixmap renderSvgWithDividers( const QString& keyname, int width, int height, const QString& element = QString() );

        /**
         * Add nice borders to a pixmap. The function will create and return a new
         * Pixmal that is the size of the old one plus twice the border width in
         * each dimension.
         * 
         * @param orgPixmap The original pixmap.
         * @param borderWidth The pixel width of the borders to add to the pixmap. 
         * @param name A name for use as the basis of the cache key that for caching the completed image plus borders.
         * @param skipCache If true, the pixmap will always get rendered and never fetched from the cache.
         */
        QPixmap addBordersToPixmap( QPixmap orgPixmap, int borderWidth, const QString &name, bool skipCache =false );

        /**
         * Paint a custom slider using the specified painter. The slider consists
         * of a background part, a "knob" that moves along it to show the current
         * position, and 2 end markers to clearly mark the ends of the slider.
         * The background part before the knob, is painted in a different color than the
         * part after (and under) the knob.
         * @param p The painter to use.
         * @param x The x position to begin painting at.
         * @param y The y position to begin painting at.
         * @param width The width of the slider to paint.
         * @param height The height of the slider. The background part does not scale in height, it will always be a relatively thin line, but the knob and end markers do.
         * @param percentage The percentange of the slider that the knob is positioned at.
         * @param active Specifies whether the slider should be painted "active" using the current palettes active colors, to specify that it currently has mouse focus or hover.
         */
        void paintCustomSlider( QPainter *p, int x, int y, int width, int height, qreal percentage, bool active );

        /**
         * Calculate the visual slider knob rect from its value, use it instead the QStyle functions
         * QStyle::sliderPositionFromValue() and QStyle::subControlRect();
         */
        QRect sliderKnobRect( const QRect &slider, qreal percent );

        /**
         * Get the path of the currently used svg theme file.
         *
         * @return the path of the currently used theme file.
         */
        QString themeFile();

        /**
         * Change the currently used svg theme file. This function also
         * clears the pixmap cache as all svg elements have potentially changed
         * and should be re-rendered.
         *
         * @param themeFile The path of the new theme file to use.
         */
        void setThemeFile( const QString  & themeFile );

    public slots:
        void reTint();

    private:
        SvgHandler( QObject* parent = 0 );

        bool loadSvg( const QString& name );

        KPixmapCache * m_cache;

        QHash<QString,KSvgRenderer*> m_renderers;
        QReadWriteLock m_lock;

        QString m_themeFile;
        bool m_customTheme;
};

#endif

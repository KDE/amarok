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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
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
A class to abstract out some common opperations of users of tinted svgs
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
        * @param width Widht of the resulting pixmap
        * @param height Height of the resulting pixmap
        * @param element The theme element to render ( if none the entire svg is rendered )
        * @return The svg element/file rendered into a pixmap
        */
        QPixmap renderSvg( const QString& keyname, int width, int height, const QString& element = QString() );

        
        /**
         * Yet another oveloaded function. This one renders the svg element and adds half a divider element to the top and the bottom
         * so it looks sane when multiple elements with the same widh are stacked.
         * @param keyname the name of the key to save in the cache
         * @param width Widht of the resulting pixmap
         * @param height Height of the resulting pixmap
         * @param element The theme element to render ( if none the entire svg is rendered )
         * @return The svg element/file rendered into a pixmap
         */
        QPixmap renderSvgWithDividers( const QString& keyname, int width, int height, const QString& element = QString() );


        QPixmap addBordersToPixmap( QPixmap orgPixmap, int borderWidth, const QString &name, bool skipCache =false );

        void paintCustomSlider( QPainter *p, int x, int y, int width, int height, qreal percentage, bool active );

        QString themeFile();
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

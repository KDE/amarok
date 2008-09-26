/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *             (c) 2008  Jeff Mitchell <kde-dev@emailgoeshere.com>         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
 
#ifndef SVGHANDLER_H
#define SVGHANDLER_H

#include <QPixmap>
#include <QString>
#include <KSvgRenderer>

#include "amarok_export.h"

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
        
        void reTint( );

        QString themeFile();
        void setThemeFile( const QString  & themeFile );

    private:
        SvgHandler( QObject* parent );
        ~SvgHandler();

        class Private;
        Private * const d;
};

#endif

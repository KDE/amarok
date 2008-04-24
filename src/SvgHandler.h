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
#include <QSvgRenderer>

#include "amarok_export.h"

/**
A class to abstract out some common opperations of users of tinted svgs

	@author 
*/
class AMAROK_EXPORT SvgHandler{
public:

    static SvgHandler * instance();

    ~SvgHandler();

    QSvgRenderer* getRenderer( const QString &name );
    QPixmap renderSvg( const QString &name, const QString& keyname, int width, int height, const QString& element = QString() );
    void reTint( const QString &name );

private:
    friend class SvgHandlerSingleton;
    SvgHandler();

    class Private;
    Private * const d;

};

#endif

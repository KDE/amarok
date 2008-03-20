/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 
#include "SvgHandler.h"

#include "debug.h"
#include "SvgTinter.h"
#include "TheInstances.h"

#include <KStandardDirs>

#include <QPainter>
#include <QPixmapCache>

SvgHandler::SvgHandler()
    : m_svgRenderer( 0 )
    , m_svgFilename( QString() )
{
}

SvgHandler::~SvgHandler()
{
    delete m_svgRenderer;
    m_svgRenderer = 0;
}

void SvgHandler::loadSvg( QString name )
{

    m_svgFilename = KStandardDirs::locate( "data", name );
    
    m_svgRenderer = new QSvgRenderer( The::svgTinter()->tint( m_svgFilename ).toAscii() );
    if ( ! m_svgRenderer->isValid() )
        debug() << "svg file '" + m_svgFilename + "' cannot be loaded";
}


QPixmap SvgHandler::renderSvg( QString keyname, int width, int height, QString element ) const
{

    QString key = QString("%1:%2x%3")
            .arg( keyname )
            .arg( width )
            .arg( height );

    QPixmap pixmap( width, height );
    pixmap.fill( Qt::transparent );

    if ( !QPixmapCache::find(key, pixmap) ) {
//         debug() << QString("svg %1 not in cache...").arg( key );

        QPainter pt( &pixmap );
        if ( !element.isEmpty() )
            m_svgRenderer->render( &pt, element, QRectF( 0, 0, width, height ) );
        else
            m_svgRenderer->render( &pt, QRectF( 0, 0, width, height ) );
  
        QPixmapCache::insert(key, pixmap);


    }

    return pixmap;
}

void SvgHandler::reTint()
{

    The::svgTinter()->init();
    
    delete m_svgRenderer;
    m_svgRenderer = new QSvgRenderer( The::svgTinter()->tint( m_svgFilename ).toAscii() );
    if ( ! m_svgRenderer->isValid() )
        debug() << "svg file '" + m_svgFilename + "' cannot be loaded";

}



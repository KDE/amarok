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
 
#include "SvgHandler.h"

#include "debug.h"
#include "SvgTinter.h"
#include "TheInstances.h"

#include <KStandardDirs>

#include <QPainter>
#include <QPixmapCache>

SvgHandler * SvgHandler::m_instance = 0;

SvgHandler * SvgHandler::instance()
{

    if ( m_instance == 0 )
        m_instance = new SvgHandler();

    return m_instance;
}


SvgHandler::SvgHandler()
    : m_renderers()
{
}

SvgHandler::~SvgHandler()
{
}

bool SvgHandler::loadSvg( const QString& name )
{
    QString svgFilename = KStandardDirs::locate( "data", name );
    
    QSvgRenderer *renderer = new QSvgRenderer( The::svgTinter()->tint( svgFilename ).toAscii() );

    if ( ! renderer->isValid() )
    {
        debug() << "Bluddy 'ell guvna, aye canna' load ya Ess Vee Gee at " << svgFilename;
        return false;
    }

    if( m_renderers[name] )
        delete m_renderers[name];

    m_renderers[name] = renderer;
    return true;
}

QSvgRenderer* SvgHandler::getRenderer( const QString& name )
{
    if( ! m_renderers[name] )
        if( ! loadSvg( name ) )
            m_renderers[name] = new QSvgRenderer();
    return m_renderers[name];
}

QPixmap SvgHandler::renderSvg( const QString &name, const QString& keyname, int width, int height, const QString& element )
{
    QPixmap pixmap( width, height );
    pixmap.fill( Qt::transparent );

    if( ! m_renderers[name] )
        if( ! loadSvg( name ) )
            return pixmap;

    const QString key = QString("%1:%2x%3")
        .arg( keyname )
        .arg( width )
        .arg( height );


    if ( !QPixmapCache::find( key, pixmap ) ) {
//         debug() << QString("svg %1 not in cache...").arg( key );

        QPainter pt( &pixmap );
        if ( element.isEmpty() )
            m_renderers[name]->render( &pt, QRectF( 0, 0, width, height ) );
        else
            m_renderers[name]->render( &pt, element, QRectF( 0, 0, width, height ) );
  
        QPixmapCache::insert( key, pixmap );
    }

    return pixmap;
}

void SvgHandler::reTint( const QString &name )
{
    The::svgTinter()->init();
    loadSvg( name );
}

namespace The {
    AMAROK_EXPORT SvgHandler* svgHandler() { return SvgHandler::instance(); }
}


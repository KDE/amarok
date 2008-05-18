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
 
#include "WidgetBackgroundPainter.h"

#include "Debug.h"
#include "MainWindow.h"
#include "SvgHandler.h"
#include "TheInstances.h"

#include <QPixmap>
#include <QPixmapCache>
#include <QWidget>


WidgetBackgroundPainter * WidgetBackgroundPainter::m_instance = 0;

WidgetBackgroundPainter * WidgetBackgroundPainter::instance()
{

    if ( m_instance == 0 )
        m_instance = new WidgetBackgroundPainter();

    return m_instance;
}


WidgetBackgroundPainter::WidgetBackgroundPainter()
{
}


WidgetBackgroundPainter::~WidgetBackgroundPainter()
{
}

QPixmap WidgetBackgroundPainter::getBackground( QWidget * parent, int x, int y, int width, int height, bool ignoreCache )
{

    //get postion of widget in global coords.
    QPoint globalTopLeft = parent->mapToGlobal( QPoint( 0, 0 ) );
    //QPoint globalBottomRight = parent->mapToGlobal( parent->rect().bottomRight() );

    return getBackground( parent->name(), globalTopLeft.x(), globalTopLeft.y(), x, y, width, height, ignoreCache );
            
}

QPixmap WidgetBackgroundPainter::getBackground( const QString name, int globalAreaX , int globalAreaY, int x, int y, int width, int height, bool ignoreCache)
{
    //get global offset of the logical background image
    
    QPoint backgroundGlobalTopLeft = MainWindow::self()->globalBackgroundOffset();

    int globalX = backgroundGlobalTopLeft.x();
    int globalY = backgroundGlobalTopLeft.y();


    int cutoutX = globalAreaX - globalX;
    int cutoutY = globalAreaY - globalY;

    
    QString key = QString("%1_bg:%2x%3").arg( name ).arg( width ).arg( height );
    QPixmap background( width, height );

    //debug() << "key: " << key;


    if ( ignoreCache || !QPixmapCache::find( key, background ) ) {

        debug() << "cutout: " << cutoutX << ", " << cutoutY << ", " << width << ", " << height;
        
        QSize backgroundSize = MainWindow::self()->backgroundSize();
        QPixmap mainBackground = The::svgHandler()->renderSvg( "main_background", backgroundSize.width(), backgroundSize.height(), "context_wallpaper" );
        background = mainBackground.copy( cutoutX, cutoutY, width, height );
        QPixmapCache::insert( key, background );

    }

    return background;
}



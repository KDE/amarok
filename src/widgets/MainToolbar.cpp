/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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

#include "MainToolbar.h"

#include "debug.h"

#include <KStandardDirs>   

#include <QPainter>
#include <QPixmapCache>

MainToolbar::MainToolbar( QWidget * parent )
 : KHBox( parent )
{

    m_svgRenderer = new QSvgRenderer( KStandardDirs::locate( "data","amarok/images/toolbar-background.svg" ));
    if ( ! m_svgRenderer->isValid() )
        debug() << "svg is kaputski";
}


MainToolbar::~MainToolbar()
{
}

void MainToolbar::paintEvent(QPaintEvent *)
{

    int middle = contentsRect().width() / 2;
    QRect controlRect( middle - 125, 0, 250, 50 );


    QString key = QString("toolbar-background:%1x%2")
                            .arg(contentsRect().width())
                            .arg(contentsRect().height());

    QPixmap background(contentsRect().width(), contentsRect().height() );

    if (!QPixmapCache::find(key, background)) {
        debug() << QString("toolbar background %1 not in cache...").arg(key);

        QPainter pt( &background );
        m_svgRenderer->render( &pt, "toolbarbackground",  contentsRect() );
        m_svgRenderer->render( &pt, "buttonbar",  controlRect );
        QPixmapCache::insert(key, background);
    }


    QPainter painter( this );
    painter.drawPixmap( 0, 0, background );

}



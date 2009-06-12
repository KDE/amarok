/***************************************************************************
 *   Copyright (c) 2009  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
#include "BreadcrumbWidget.h"

#include "Debug.h"

#include <KLocale>

BreadcrumbWidget::BreadcrumbWidget( QWidget * parent )
    : KHBox( parent)
{
    setFixedHeight( 28 );
    m_rootItem = new QPushButton( i18n( "Home / " ), this );
}


BreadcrumbWidget::~BreadcrumbWidget()
{
}

void BreadcrumbWidget::setPath( const QString &path )
{
    DEBUG_BLOCK
    debug() << "got path: " << path;

    //remove name of the root item as we provide our own.
    QString modifiedPath = path;
    modifiedPath.replace( "root list/", "" );
    
    m_currentPath = modifiedPath.split( '/' );
    updateBreadcrumbs();
}

void BreadcrumbWidget::updateBreadcrumbs()
{
    DEBUG_BLOCK
    qDeleteAll( m_items );
    m_items.clear();

    foreach( QString item, m_currentPath )
    {
        QPushButton * label = new QPushButton( item + " / ", this );
        m_items.append( label );
    }
}



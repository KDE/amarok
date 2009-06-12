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

    setStyleSheet( "QPushButton { border: none; }"
                   "QPushButton:hover { background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #f6f7fa, stop: 1 #dadbde); }"
                 );

    
    m_rootItem = new QPushButton( i18n( "Home / " ), this );
    m_rootItem->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

    connect( m_rootItem, SIGNAL( clicked( bool ) ), this, SLOT( rootItemClicked() ) );

    setContentsMargins( 0, 0, 0, 0 );
    setSpacing( 0 );

    m_spacer = new QWidget( 0 );
}


BreadcrumbWidget::~BreadcrumbWidget()
{
}

void BreadcrumbWidget::setPath( const QString &path )
{
    DEBUG_BLOCK
    debug() << "got path: " << path;

    if ( path == "root list" )
    {
        m_currentPath.clear();
    }
    else
    {
        //remove name of the root item as we provide our own.
        QString modifiedPath = path;
        modifiedPath.replace( "root list/", "" );
        m_currentPath = modifiedPath.split( '/' );
    }

    updateBreadcrumbs();
}

void BreadcrumbWidget::updateBreadcrumbs()
{
    DEBUG_BLOCK
    qDeleteAll( m_items );
    m_items.clear();

    m_spacer->setParent( 0 );
    
    foreach( QString item, m_currentPath )
    {
        QPushButton * label = new QPushButton( item + " / ", this );
        label->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
        m_items.append( label );
    }

    m_spacer->setParent( this );
}

void BreadcrumbWidget::rootItemClicked()
{
    DEBUG_BLOCK
    emit toHome();
}

#include "BreadcrumbWidget.moc"


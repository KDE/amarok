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
    , m_rootList( 0 )
{
    setFixedHeight( 28 );

    setStyleSheet( "QPushButton { border: none; }"
                   "QPushButton:hover { background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #f6f7fa, stop: 1 #dadbde); }"
                   "QPushButton::menu-indicator { image: none; width: 0px;}"
                 );

    setContentsMargins( 3, 0, 3, 0 );
    setSpacing( 0 );

    m_spacer = new QWidget( 0 );
}


BreadcrumbWidget::~BreadcrumbWidget()
{
}

void
BreadcrumbWidget::setRootList( BrowserCategoryList * rootList )
{
    m_rootList = rootList;

    //update the breadcrumbs every time the view changes.
    connect( m_rootList, SIGNAL( viewChanged() ), this, SLOT( updateBreadcrumbs() ) );

    updateBreadcrumbs();
}

void
BreadcrumbWidget::updateBreadcrumbs()
{
    DEBUG_BLOCK

    if ( !m_rootList )
        return;

    debug() << "going to delete " << m_items.size() << " items";
    qDeleteAll( m_items );
    m_items.clear();
    debug() << "deleted!";

    m_spacer->setParent( 0 );

    addLevel( m_rootList );

    m_spacer->setParent( this );
}

void
BreadcrumbWidget::addLevel( BrowserCategoryList * list )
{

    DEBUG_BLOCK
    QString prettyName = list->prettyName();

    BrowserCategory * childCategory = list->activeCategory();

    if ( childCategory )
    {
        BreadcrumbItem * branch = new BreadcrumbItem( prettyName, list, this );
        m_items.append( branch );
        
        //check if this is also a list
        BrowserCategoryList *childList = dynamic_cast<BrowserCategoryList*>( childCategory );
        if ( childList )
        {
            addLevel( childList );
        }
        else
        {
            BreadcrumbItem * leaf = new BreadcrumbItem( childCategory->prettyName(), childCategory, this );
            m_items.append( leaf );

            leaf->setBold( true );
        }
    }
    else
    {
        BreadcrumbItem * item = new BreadcrumbItem( prettyName, list, this );
        m_items.append( item );
        item->setBold( true );
    }
}

#include "BreadcrumbWidget.moc"

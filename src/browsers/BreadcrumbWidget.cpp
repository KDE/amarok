/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#include "BreadcrumbWidget.h"

#include "BreadcrumbItemButton.h"

#include "Debug.h"

#include <KLocale>

BreadcrumbWidget::BreadcrumbWidget( QWidget * parent )
    : KHBox( parent)
    , m_rootList( 0 )
{
    setFixedHeight( 28 );
    setContentsMargins( 3, 0, 3, 0 );
    setSpacing( 0 );

    m_breadcrumbArea = new KHBox( this );
    m_breadcrumbArea->setContentsMargins( 0, 0, 0, 0 );
    m_breadcrumbArea->setSpacing( 0 );
    setStretchFactor( m_breadcrumbArea, 10 );

    new BreadcrumbUrlMenuButton( "navigate", this );

    m_spacer = new QWidget( 0 );
}

BreadcrumbWidget::~BreadcrumbWidget()
{
    clearCrumbs();
}

void
BreadcrumbWidget::clearCrumbs()
{
    //these items will get deleted by their BrowserCategory, so set parent to 0
    //or they will get double deleted, causing a crash
    foreach( BreadcrumbItem *item, m_items )
    {
        item->hide();
        item->setParent( 0 );
    }
    m_items.clear();
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

    if( !m_rootList )
        return;

    clearCrumbs();
    m_spacer->setParent( 0 );
    addLevel( m_rootList );
    m_spacer->setParent( m_breadcrumbArea );
}

void
BreadcrumbWidget::addLevel( BrowserCategoryList * list )
{
    DEBUG_BLOCK
    BreadcrumbItem *item = list->breadcrumb();
    item->setParent( m_breadcrumbArea );
    item->show();
    m_items.append( item );

    BrowserCategory *childCategory = list->activeCategory();

    if( childCategory )
    {
        item->setActive( false );
        
        //check if this is also a list
        BrowserCategoryList *childList = dynamic_cast<BrowserCategoryList*>( childCategory );
        if( childList )
        {
            addLevel( childList );
        }
        else
        {
            BreadcrumbItem * leaf = childCategory->breadcrumb();
            leaf->setParent( m_breadcrumbArea );
            leaf->show();
            leaf->setActive( true );
            
            m_items.append( leaf );
        }
    }
    else
    {
        item->setActive( true );
    }
}

#include "BreadcrumbWidget.moc"

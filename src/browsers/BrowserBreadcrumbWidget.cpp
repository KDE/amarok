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

#include "BrowserBreadcrumbWidget.h"

#include "widgets/BreadcrumbItemButton.h"

#include "Debug.h"

#include <KLocale>

BrowserBreadcrumbWidget::BrowserBreadcrumbWidget( QWidget * parent )
    : KHBox( parent)
    , m_rootList( 0 )
    , m_childMenuButton( 0 )
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

BrowserBreadcrumbWidget::~BrowserBreadcrumbWidget()
{
    clearCrumbs();
}

void
BrowserBreadcrumbWidget::clearCrumbs()
{
    //these items will get deleted by their BrowserCategory, so set parent to 0
    //or they will get double deleted, causing a crash
    foreach( BrowserBreadcrumbItem *item, m_items )
    {
        item->hide();
        item->setParent( 0 );
    }
    m_items.clear();

    //if we have a final menu button, also delete it.
    delete m_childMenuButton;
    m_childMenuButton = 0;
    
}

void
BrowserBreadcrumbWidget::setRootList( BrowserCategoryList * rootList )
{
    m_rootList = rootList;

    //update the breadcrumbs every time the view changes.
    connect( m_rootList, SIGNAL( viewChanged() ), this, SLOT( updateBreadcrumbs() ) );

    updateBreadcrumbs();
}

void
BrowserBreadcrumbWidget::updateBreadcrumbs()
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
BrowserBreadcrumbWidget::addLevel( BrowserCategoryList * list )
{
    DEBUG_BLOCK
    BrowserBreadcrumbItem *item = list->breadcrumb();
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
            BrowserBreadcrumbItem * leaf = childCategory->breadcrumb();
            leaf->setParent( m_breadcrumbArea );
            leaf->show();
            leaf->setActive( true );
            
            m_items.append( leaf );
        }
    }
    else
    {
        item->setActive( true );

        //if this item has children, add a menu button for selecting these.
        BrowserCategoryList *childList = dynamic_cast<BrowserCategoryList*>( list );
        if( childList )
        {
            m_childMenuButton = new BreadcrumbItemMenuButton( m_breadcrumbArea );

            QMenu *menu = new QMenu( 0 );

            QMap<QString,BrowserCategory *> childMap =  childList->categories();

            QStringList childNames = childMap.keys();

            foreach( QString siblingName, childNames )
            {
                //no point in adding ourselves to this menu
                if ( siblingName == list->name() )
                    continue;

                BrowserCategory * siblingCategory = childMap.value( siblingName );

                QAction * action = menu->addAction( siblingCategory->icon(), siblingCategory->prettyName() );
                connect( action, SIGNAL( triggered() ), childMap.value( siblingName ), SLOT( activate() ) );

            }

        m_childMenuButton->setMenu( menu );
        
        //do a little magic to line up items in the menu with the current item
        int offset = 6;
        menu->setContentsMargins( offset, 1, 1, 2 );

        }
        
    }
}

#include "BrowserBreadcrumbWidget.moc"

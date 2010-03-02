/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "BrowserBreadcrumbWidget.h"

#include "Debug.h"
#include "widgets/BreadcrumbItemButton.h"

#include <KLocale>

#include <QResizeEvent>

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
            debug() << "here!";
            BrowserBreadcrumbItem * leaf = childCategory->breadcrumb();
            leaf->setParent( m_breadcrumbArea );
            leaf->show();
            leaf->setActive( true );
            
            m_items.append( leaf );


            //no children, but check if there are additional breadcrumb levels (for internal navigation in the category) that should be added anyway.
            foreach( BrowserBreadcrumbItem * addItem, childCategory->additionalItems() )
            {
                debug() << "adding extra item";

                //hack to ensure that we have not already added it to the front of the breadcrumb...
                addItem->setParent( 0 );
                
                addItem->setParent( m_breadcrumbArea );
                addItem->show();
                addItem->setActive( true );
            }
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

            foreach( const QString &siblingName, childNames )
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

    hideAsNeeded( width() );
}

void BrowserBreadcrumbWidget::resizeEvent( QResizeEvent * event )
{
    hideAsNeeded( event->size().width() );
}

void BrowserBreadcrumbWidget::hideAsNeeded( int width )
{

    DEBUG_BLOCK

    //we need to check if there is enough space for all items, if not, we start hiding items from the left (excluding the home item) until they fit (we never hide the rightmost item)
    //we also add he hidden levels to the drop down menu of the last item so they are accessible.


    //make a temp list that includes both regular items and add items
    QList<BrowserBreadcrumbItem *> allItems;

    allItems.append( m_items );
    if( m_rootList->activeCategory() != 0 )
        allItems.append( m_rootList->activeCategory()->additionalItems() );

    debug() << "the active category is: " <<  m_rootList->activeCategoryName();
    

    int sizeOfFirst = allItems.first()->nominalWidth();
    int sizeOfLast = allItems.last()->nominalWidth();

    int spaceLeft = width - ( sizeOfFirst + sizeOfLast + 28 );

    int numberOfItems = allItems.count();
    debug() << numberOfItems << " items.";

    for( int i = numberOfItems - 2; i > 0; i-- )
    {
        debug() << "item index " << i << " has width " << allItems.at( i )->nominalWidth() << " and space left is " << spaceLeft;
        
        if( allItems.at( i )->nominalWidth() <= spaceLeft )
        {
            allItems.at( i )->show();
            spaceLeft -= allItems.at( i )->nominalWidth();
        }
        else
        {
            //set spaceLeft to 0 so no items further to the left are shown
            spaceLeft = 0;
            allItems.at( i )->hide();
        }
    }
}


#include "BrowserBreadcrumbWidget.moc"

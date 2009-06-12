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

#include <QMenu>


BreadcrumbItem::BreadcrumbItem( const QString & name, BrowserCategory * category, QWidget * parent )
    : KHBox( parent )
    , m_category( category )
    , m_menuButton( 0 )
{

    //figure out if we want to add a menu to this item. A menu allows you to select
    //any of the _sibling_ items. (yes, I know, this is different from how Dolphin
    //does it, but I find the Dolphin way amazingly unintuitive and I always get it
    //wrong when using it...)
    
    BrowserCategoryList * parentList = category->parentList();
    if ( parentList )
    {
        m_menuButton = new QPushButton( " > ", this );
        m_menuButton->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

        QMenu * menu = new QMenu( this );
        
        QMap<QString,BrowserCategory *> siblingMap =  parentList->categories();

        QStringList siblingNames = siblingMap.keys();

        foreach( QString siblingName, siblingNames )
        {
            QAction * action = menu->addAction( siblingMap.value( siblingName )->prettyName() );
            connect( action, SIGNAL( triggered() ), siblingMap.value( siblingName ), SLOT( activate() ) );
        }

        m_menuButton->setMenu( menu );
    }


    m_mainButton = new QPushButton( name, this );
    m_mainButton->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

    setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    
    //if this is a list, make cliking on this item cause us
    //to navigate to its home.
    BrowserCategoryList *list = dynamic_cast<BrowserCategoryList*>( category );
    if ( list )
    {
        connect( m_mainButton, SIGNAL( clicked( bool ) ), list, SLOT( home() ) );
    }

}

BreadcrumbItem::~BreadcrumbItem()
{
    DEBUG_BLOCK
}

void BreadcrumbItem::setBold( bool bold )
{
    QFont font = m_mainButton->font();
    font.setBold( true );
    m_mainButton->setFont( font );
}


BreadcrumbWidget::BreadcrumbWidget( QWidget * parent )
    : KHBox( parent)
    , m_rootList( 0 )
{
    setFixedHeight( 28 );

    setStyleSheet( "QPushButton { border: none; }"
                   "QPushButton:hover { background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #f6f7fa, stop: 1 #dadbde); }"
                   "QPushButton::menu-indicator { image: none; width: 0px;}"
                 );

    setContentsMargins( 0, 0, 0, 0 );
    setSpacing( 0 );

    m_spacer = new QWidget( 0 );
}


BreadcrumbWidget::~BreadcrumbWidget()
{
}

void BreadcrumbWidget::setRootList( BrowserCategoryList * rootList )
{
    m_rootList = rootList;

    //update the breadcrumbs every time the view changes.
    connect( m_rootList, SIGNAL( viewChanged() ), this, SLOT( updateBreadcrumbs() ) );
    
    updateBreadcrumbs();
}

void BreadcrumbWidget::updateBreadcrumbs()
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

void BreadcrumbWidget::addLevel( BrowserCategoryList * list )
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


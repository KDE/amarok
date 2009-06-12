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


BreadcrumbItem::BreadcrumbItem( const QString & name, BrowserCategory * category, QWidget * parent )
    : QPushButton( name, parent )
    , m_category( category )
{
    //if this is a list, make cliking on this item cause us
    //to navigate to its home.
    BrowserCategoryList *list = dynamic_cast<BrowserCategoryList*>( category );
    if ( list )
    {
        connect( this, SIGNAL( clicked( bool ) ), list, SLOT( home() ) );
    }

    setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
}

BreadcrumbItem::~ BreadcrumbItem()
{
}

BreadcrumbWidget::BreadcrumbWidget( QWidget * parent )
    : KHBox( parent)
    , m_rootList( 0 )
{
    setFixedHeight( 28 );

    setStyleSheet( "QPushButton { border: none; }"
                   "QPushButton:hover { background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #f6f7fa, stop: 1 #dadbde); }"
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
            
    qDeleteAll( m_items );
    m_items.clear();

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
        //we are going to add more, so add a seperator
        prettyName += " / ";
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

            QFont font = leaf->font();
            font.setBold( true );
            leaf->setFont( font );
        }
    }
    else
    {
        BreadcrumbItem * item = new BreadcrumbItem( prettyName, list, this );

        m_items.append( item );
        
        QFont font = item->font();
        font.setBold( true );
        item->setFont( font );
    }
}




#include "BreadcrumbWidget.moc"


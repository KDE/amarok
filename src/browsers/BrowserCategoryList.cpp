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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "BrowserCategoryList.h"

#include "Debug.h"
#include "BrowserCategoryListDelegate.h"
#include "context/ContextView.h"
#include "PaletteHandler.h"
#include "widgets/PrettyTreeView.h"
#include "widgets/SearchWidget.h"

#include <KLineEdit>

BrowserCategoryList::BrowserCategoryList( QWidget * parent, const QString& name )
    : BrowserCategory( name )
    , m_currentCategory( 0 )
    , m_categoryListModel( new BrowserCategoryListModel() )
{
    setObjectName( name );
    debug() << "BrowserCategoryList starting...";

    m_searchWidget = new SearchWidget( this, this, false );

    m_filterTimer.setSingleShot( true );
    connect( &m_filterTimer, SIGNAL( timeout() ), this, SLOT( slotFilterNow() ) );

    m_categoryListView = new Amarok::PrettyTreeView( this );
#ifdef Q_WS_MAC
    m_categoryListView->setVerticalScrollMode( QAbstractItemView::ScrollPerItem ); // for some bizarre reason w/ some styles on mac
    m_categoryListView->setHorizontalScrollMode( QAbstractItemView::ScrollPerItem ); // per-pixel scrolling is slower than per-item
#else
    m_categoryListView->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel ); // Scrolling per item is really not smooth and looks terrible
    m_categoryListView->setHorizontalScrollMode( QAbstractItemView::ScrollPerPixel ); // Scrolling per item is really not smooth and looks terrible
#endif
    
    m_categoryListView->setFrameShape( QFrame::NoFrame );

    m_proxyModel = new BrowserCategoryListSortFilterProxyModel( this );
    m_proxyModel->setSourceModel( m_categoryListModel );

    m_delegate = new BrowserCategoryListDelegate( m_categoryListView );
    m_categoryListView->setItemDelegate( m_delegate );
    m_categoryListView->setSelectionMode( QAbstractItemView::NoSelection );
    m_categoryListView->setHeaderHidden( true );
    m_categoryListView->setRootIsDecorated( false );
    m_categoryListView->setSortingEnabled( true );
    m_categoryListView->setAlternatingRowColors( true );
    m_categoryListView->setModel( m_proxyModel );
    connect( m_categoryListView, SIGNAL( activated( const QModelIndex & ) ), this, SLOT( categoryActivated( const QModelIndex & ) ) );

    The::paletteHandler()->updateItemView( m_categoryListView );

    setFrameShape( QFrame::StyledPanel );
    setFrameShadow( QFrame::Sunken );
}


BrowserCategoryList::~BrowserCategoryList()
{
    DEBUG_BLOCK
    qDeleteAll( m_categories.values() );
    delete m_categoryListView;
    delete m_categoryListModel;
    delete m_delegate;
}

void
BrowserCategoryList::addCategory( BrowserCategory * category )
{
    if( !category )
        return;

    //insert service into service map
    m_categories[category->name()] = category;
    m_categoryListModel->addCategory( category );
    connect( category, SIGNAL( home() ), this, SLOT( home() ) );
}

void
BrowserCategoryList::categoryActivated( const QModelIndex & index )
{
    DEBUG_BLOCK
    BrowserCategory * category = 0;

    if ( index.data( CustomCategoryRoles::CategoryRole ).canConvert<BrowserCategory *>() )
        category = index.data( CustomCategoryRoles::CategoryRole ).value<BrowserCategory *>();
    else
        return;

    if ( category )
    {
        debug() << "Show service: " <<  category->name();
        showCategory( category->name() );
    }
}

void
BrowserCategoryList::showCategory( const QString &name )
{
    BrowserCategory * category = 0;
    if ( m_categories.contains( name ) )
        category = m_categories.value( name );

    if ( category != 0 && category != m_currentCategory )
    {
        //if a service is already shown, make damn sure to deactivate that one first...
        if ( m_currentCategory )
            m_currentCategory->setParent( 0 );

        m_categoryListView->setParent( 0 );
        category->setParent ( this );
        category->move( QPoint( 0, 0 ) );
        category->show();
        category->polish();
        m_currentCategory = category;
    }

    m_searchWidget->hide();
}

void
BrowserCategoryList::home()
{
    if ( m_currentCategory != 0 )
    {
        m_currentCategory->setParent( 0 );
        m_categoryListView->setParent( this );
        m_currentCategory = 0; // remove any context stuff we might have added
        m_searchWidget->show();
    }
}


QMap< QString, BrowserCategory * >
BrowserCategoryList::categories()
{
    return m_categories;
}

void
BrowserCategoryList::removeCategory( const QString &name )
{
    DEBUG_BLOCK
    debug() << "removing category: " << name;
    BrowserCategory * category = m_categories.take( name );
    if ( m_currentCategory == category )
        home();

    if( category )
        m_categoryListModel->removeCategory( category );
    delete category;
    m_categoryListView->reset();
}

void BrowserCategoryList::slotSetFilterTimeout()
{
    KLineEdit *lineEdit = dynamic_cast<KLineEdit*>( sender() );
    if( lineEdit )
    {
        m_currentFilter = lineEdit->text();
        m_filterTimer.stop();
        m_filterTimer.start( 500 );
    }
}

void BrowserCategoryList::slotFilterNow()
{
    m_proxyModel->setFilterFixedString( m_currentFilter );
}

QString BrowserCategoryList::activeCategoryName()
{
    DEBUG_BLOCK
    if ( m_currentCategory )
        return m_currentCategory->name();
    return QString();
}

BrowserCategory * BrowserCategoryList::activeCategory()
{
    return m_currentCategory;
}


#include "BrowserCategoryList.moc"




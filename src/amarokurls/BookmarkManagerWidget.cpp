/****************************************************************************************
 * Copyright (c) 2008, 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>              *
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

#include "BookmarkManagerWidget.h"

#include "AmarokUrl.h"
#include "BookmarkModel.h"
#include "NavigationUrlGenerator.h"
#include "PlayUrlGenerator.h"
#include "ProgressWidget.h"

#include <KAction>
#include <KIcon>
#include <KLocale>
#include <KVBox>

#include <QLabel>

BookmarkManagerWidget::BookmarkManagerWidget( QWidget * parent )
 : KVBox( parent )
{

    setContentsMargins( 0,0,0,0 );

    KHBox * topLayout = new KHBox( this );
    
    m_toolBar = new QToolBar( topLayout );
    m_toolBar->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );

    KAction * addGroupAction = new KAction( KIcon("media-track-add-amarok" ), i18n( "Add Folder" ), this  );
    m_toolBar->addAction( addGroupAction );
    connect( addGroupAction, SIGNAL( triggered( bool ) ), BookmarkModel::instance(), SLOT( createNewGroup() ) );

    KAction * addBookmarkAction = new KAction( KIcon("bookmark-new" ), i18n( "New Bookmark" ), this  );
    m_toolBar->addAction( addBookmarkAction );
    connect( addBookmarkAction, SIGNAL( triggered( bool ) ), BookmarkModel::instance(), SLOT( createNewBookmark() ) );


    m_searchEdit = new Amarok::LineEdit( topLayout );
    m_searchEdit->setClickMessage( i18n( "Filter bookmarks" ) );
    m_searchEdit->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    m_searchEdit->setClearButtonShown( true );
    m_searchEdit->setFrame( true );
    m_searchEdit->setToolTip( i18n( "Start typing to progressively filter the bookmarks" ) );
    m_searchEdit->setFocusPolicy( Qt::ClickFocus ); // Without this, the widget goes into text input mode directly on startup

    connect( m_searchEdit, SIGNAL( textChanged( const QString & ) ), this, SLOT( slotFilterChanged(  const QString &  ) ) );
    
    m_bookmarkView = new BookmarkTreeView( this );

    m_proxyModel = new QSortFilterProxyModel( this );
    m_proxyModel->setSourceModel( BookmarkModel::instance() );
    m_proxyModel->setFilterCaseSensitivity( Qt::CaseInsensitive );
    m_proxyModel->setSortCaseSensitivity( Qt::CaseInsensitive );
    m_proxyModel->setDynamicSortFilter( true );
    m_proxyModel->setFilterKeyColumn ( -1 ); //filter on all columns

    m_bookmarkView->setModel( m_proxyModel );
    m_bookmarkView->setProxy( m_proxyModel );
    m_bookmarkView->setSortingEnabled( true );
    m_bookmarkView->resizeColumnToContents( 0 );

    connect( BookmarkModel::instance(), SIGNAL( editIndex( const QModelIndex & ) ), m_bookmarkView, SLOT( slotEdit( const QModelIndex & ) ) );

    m_currentBookmarkId = -1;

}

BookmarkManagerWidget::~BookmarkManagerWidget()
{
}


BookmarkTreeView * BookmarkManagerWidget::treeView()
{
    return m_bookmarkView;
}

void BookmarkManagerWidget::slotFilterChanged( const QString &filter )
{

    //when the clear button is pressed, we get 2 calls to this slot... filter this out  as it messes with
    //resetting the view:
    if ( filter == m_lastFilter )
        return;

    m_proxyModel->setFilterFixedString( filter );
    
}


#include "BookmarkManagerWidget.moc"




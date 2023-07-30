/****************************************************************************************
 * Copyright (c) 2008, 2009 Nikolaj Hald Nielsen <nhn@kde.org>                          *
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

#include "BookmarkManagerWidget.h"

#include "amarokurls/AmarokUrl.h"
#include "amarokurls/BookmarkModel.h"
#include "amarokurls/BookmarkCurrentButton.h"
#include "amarokurls/NavigationUrlGenerator.h"
#include "amarokurls/PlayUrlGenerator.h"
#include "widgets/ProgressWidget.h"

#include <QAction>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QVBoxLayout>

#include <KLocalizedString>

BookmarkManagerWidget::BookmarkManagerWidget( QWidget * parent )
    : BoxWidget( true, parent )
{
    layout()->setContentsMargins( 0,0,0,0 );

    BoxWidget * topLayout = new BoxWidget( false, this );
    
    m_toolBar = new QToolBar( topLayout );
    m_toolBar->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );

    QAction * addGroupAction = new QAction( QIcon::fromTheme(QStringLiteral("media-track-add-amarok") ), i18n( "Add Group" ), this  );
    m_toolBar->addAction( addGroupAction );
    connect( addGroupAction, &QAction::triggered, BookmarkModel::instance(), &BookmarkModel::createNewGroup );

    /*QAction * addBookmarkAction = new QAction( QIcon::fromTheme("bookmark-new" ), i18n( "New Bookmark" ), this  );
    m_toolBar->addAction( addBookmarkAction );
    connect( addBookmarkaction, &QAction::triggered, BookmarkModel::instance(), &BookmarkModel::createNewBookmark );*/

    m_toolBar->addWidget( new BookmarkCurrentButton( nullptr ) );

    m_searchEdit = new Amarok::LineEdit( topLayout );
    m_searchEdit->setPlaceholderText( i18n( "Filter bookmarks" ) );
    m_searchEdit->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    m_searchEdit->setClearButtonEnabled( true );
    m_searchEdit->setFrame( true );
    m_searchEdit->setToolTip( i18n( "Start typing to progressively filter the bookmarks" ) );
    m_searchEdit->setFocusPolicy( Qt::ClickFocus ); // Without this, the widget goes into text input mode directly on startup

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

    connect( BookmarkModel::instance(), &BookmarkModel::editIndex, m_bookmarkView, &BookmarkTreeView::slotEdit );
    connect( m_searchEdit, &Amarok::LineEdit::textChanged, m_proxyModel, &QSortFilterProxyModel::setFilterFixedString );

    m_currentBookmarkId = -1;

}

BookmarkManagerWidget::~BookmarkManagerWidget()
{
}


BookmarkTreeView * BookmarkManagerWidget::treeView()
{
    return m_bookmarkView;
}





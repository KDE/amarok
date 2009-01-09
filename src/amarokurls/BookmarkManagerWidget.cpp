/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 
#include "BookmarkManagerWidget.h"

#include "AmarokUrl.h"
#include "BookmarkModel.h"
#include "NavigationUrlGenerator.h"

#include <KAction>
#include <KIcon>
#include <KLocale>
#include <KVBox>

#include <QLabel>

BookmarkManagerWidget::BookmarkManagerWidget( QWidget * parent )
 : KVBox( parent )
{
    
    setContentsMargins(0,0,0,0);
    
    m_toolBar = new QToolBar( this );
    m_toolBar->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );

    KAction * addGroupAction = new KAction( KIcon("media-track-add-amarok" ), i18n( "Add Folder" ), this  );
    m_toolBar->addAction( addGroupAction );
    connect( addGroupAction, SIGNAL( triggered( bool ) ), BookmarkModel::instance(), SLOT( createNewGroup() ) );

    m_bookmarkView = new BookmarkTreeView( this );
    m_bookmarkView->setModel( BookmarkModel::instance() );
    connect( m_bookmarkView, SIGNAL( bookmarkSelected( AmarokUrl ) ), this, SLOT( slotBookmarkSelected( AmarokUrl ) ) );

    KHBox * editBox1 = new KHBox( this );
    new QLabel( i18n( "Name:" ), editBox1 );
    m_currentBookmarkNameEdit = new QLineEdit( editBox1 );

    KHBox * editBox2 = new KHBox( this );
    new QLabel( i18n( "Url:" ), editBox2 );
    m_currentBookmarkUrlEdit = new QLineEdit( editBox2 );
    
    KHBox * buttonBox = new KHBox( this );
    
    m_getCurrentBookmarkButton = new QPushButton( i18n( "Get Current" ), buttonBox );
    connect( m_getCurrentBookmarkButton, SIGNAL( clicked( bool ) ), this, SLOT( showCurrentUrl() ) );
    
    m_addBookmarkButton = new QPushButton( i18n( "Add" ), buttonBox );
    connect( m_addBookmarkButton, SIGNAL( clicked( bool ) ), this, SLOT( bookmarkCurrent() ) );

    m_gotoBookmarkButton = new QPushButton( i18n( "Goto" ), buttonBox );
    connect( m_gotoBookmarkButton, SIGNAL( clicked( bool ) ), this, SLOT( gotoBookmark() ) );



}

BookmarkManagerWidget::~BookmarkManagerWidget()
{
}

void BookmarkManagerWidget::showCurrentUrl()
{
    m_currentBookmarkUrlEdit->setText( getBookmarkUrl() );
    m_currentBookmarkNameEdit->setText( i18n( "New Bookmark" ) );
}

void BookmarkManagerWidget::addBookmark()
{
}

QString BookmarkManagerWidget::getBookmarkUrl()
{
    NavigationUrlGenerator urlGenerator;
    return urlGenerator.CreateAmarokUrl().url();
}

void BookmarkManagerWidget::gotoBookmark()
{
    AmarokUrl url( m_currentBookmarkUrlEdit->text() );
    url.run();
}

void BookmarkManagerWidget::bookmarkCurrent()
{
    DEBUG_BLOCK
            
    AmarokUrl url( m_currentBookmarkUrlEdit->text() );
    url.setName( m_currentBookmarkNameEdit->text() );
    
    url.saveToDb();
    BookmarkModel::instance()->reloadFromDb();
}

void BookmarkManagerWidget::slotBookmarkSelected( AmarokUrl bookmark )
{
    m_currentBookmarkUrlEdit->setText( bookmark.url() );
    m_currentBookmarkNameEdit->setText( bookmark.name() );
}


#include "BookmarkManagerWidget.moc"


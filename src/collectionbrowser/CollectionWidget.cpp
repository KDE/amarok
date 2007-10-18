/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "CollectionWidget.h"
#include "CollectionTreeView.h"
#include "querybuilder.h"

#include <QVBoxLayout>

#include <KLineEdit>
#include <KMenu>
#include <KMenuBar>
#include "searchwidget.h"

CollectionWidget *CollectionWidget::s_instance = 0;

CollectionWidget::CollectionWidget( const char* name )
{
    s_instance = this;
    setObjectName( name );
    QVBoxLayout* layout = new QVBoxLayout;

//     KActionMenu *menubar = new KActionMenu( i18n( "Group By" ), this );
    KMenuBar *menubar = new KMenuBar;
    QMenu *filterMenu = menubar->addMenu( i18n( "Group By" ) );
    layout->addWidget( menubar );

    m_treeView = new CollectionTreeView( this );
//     menubar->setDelayed( false );

    QAction *action = new QAction( i18n("Artist"), menubar );
    connect( action, SIGNAL(triggered(bool)), SLOT(sortByArtist() ) );
    filterMenu->addAction( action );

    action = new QAction( i18n( "Artist / Album" ), menubar );
    connect( action, SIGNAL(triggered(bool)), SLOT(sortByArtistAlbum() ) );
    filterMenu->addAction( action );

    action = new QAction( i18n( "Artist / Year - Album" ), menubar ); // This was Artist / Year - Album, but would require more logic to make it work so keep it like this for now
    connect( action, SIGNAL(triggered(bool)), SLOT(sortByArtistYearAlbum() ) );
    filterMenu->addAction( action );

    action = new QAction( i18n( "Album" ), menubar );
    connect( action, SIGNAL(triggered(bool)), SLOT( sortByAlbum() ) );
    filterMenu->addAction( action );

    action = new QAction( i18n( "Genre / Artist" ), menubar );
    connect( action, SIGNAL(triggered(bool)), SLOT( sortByGenreArtist() ) );
    filterMenu->addAction( action );

    action = new QAction( i18n( "Genre / Artist / Album" ), menubar );
    connect( action, SIGNAL(triggered(bool)), SLOT(sortByGenreArtistAlbum() ) );
    filterMenu->addAction( action );

    //layout->addWidget( new KLineEdit( this ) );
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget( new SearchWidget( this, m_treeView ) );
    layout->addWidget( m_treeView );
    setLayout( layout );
    m_treeView->setShowTrackNumbers( true );
}

void
CollectionWidget::sortByArtist()
{
    m_treeView->setShowYears( false );
    m_treeView->setLevels( QList<int>() << QueryBuilder::tabArtist );
}

void
CollectionWidget::sortByArtistAlbum()
{
    m_treeView->setShowYears( false );
    m_treeView->setLevels( QList<int>() << QueryBuilder::tabArtist << QueryBuilder::tabAlbum );
}

void
CollectionWidget::sortByArtistYearAlbum()
{
    m_treeView->setShowYears( true );
    m_treeView->setLevels( QList<int>() << QueryBuilder::tabArtist << QueryBuilder::tabYear << QueryBuilder::tabAlbum );
}

void
CollectionWidget::sortByAlbum()
{
    m_treeView->setShowYears( false );
    m_treeView->setLevels( QList<int>() << QueryBuilder::tabAlbum );
}

void
CollectionWidget::sortByGenreArtist()
{
    m_treeView->setShowYears( false );
    m_treeView->setLevels( QList<int>() << QueryBuilder::tabGenre << QueryBuilder::tabArtist );
}

void
CollectionWidget::sortByGenreArtistAlbum()
{
    m_treeView->setShowYears( false );
    m_treeView->setLevels( QList<int>() << QueryBuilder::tabGenre << QueryBuilder::tabArtist << QueryBuilder::tabAlbum );
}


#include "CollectionWidget.moc"

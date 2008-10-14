/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#include "CollectionWidget.h"
#include "CollectionTreeView.h"
#include "browsers/collectionbrowser/CollectionTreeItemModel.h"
#include "SearchWidget.h"

#include <KConfigGroup>
#include <KLineEdit>
#include <KLocale>
#include <KMenu>
#include <KMenuBar>

#include <QVBoxLayout>

CollectionWidget *CollectionWidget::s_instance = 0;

CollectionWidget::CollectionWidget( const char* name , QWidget *parent )
    : KVBox(parent)
{
    s_instance = this;
    setObjectName( name );
    setMargin( 0 );
    setSpacing( 0 );

    KMenuBar *menubar = new KMenuBar( this );
    QMenu *filterMenu = menubar->addMenu( i18n( "Group By" ) );

    SearchWidget *sw = new SearchWidget( this );

    m_treeView = new CollectionTreeView( this );
    m_treeView->setFrameShape( QFrame::NoFrame );

    QList<int> cats = Amarok::config( "Collection Browser" ).readEntry( "TreeCategory", QList<int>() );
    if ( cats.isEmpty() )
        cats << CategoryId::Artist << CategoryId::Album;

    m_treeView->setModel( new CollectionTreeItemModel( cats ) );
    sw->setup( m_treeView );

    QAction *action = new QAction( i18n("Artist"), menubar );
    connect( action, SIGNAL(triggered(bool)), SLOT(sortByArtist() ) );
    filterMenu->addAction( action );

    action = new QAction( i18n( "Artist / Album" ), menubar );
    connect( action, SIGNAL(triggered(bool)), SLOT(sortByArtistAlbum() ) );
    filterMenu->addAction( action );

    action = new QAction( i18n( "Artist / Year - Album" ), menubar );
    connect( action, SIGNAL(triggered(bool)), SLOT( sortByArtistYearAlbum() ) );
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

    m_treeView->setShowTrackNumbers( true );

    setFrameShape( QFrame::StyledPanel );
    setFrameShadow( QFrame::Sunken );
}

void
CollectionWidget::sortByArtist()
{
    m_treeView->setShowYears( false );
    m_treeView->setLevels( QList<int>() << CategoryId::Artist );
}

void
CollectionWidget::sortByArtistAlbum()
{
    m_treeView->setShowYears( false );
    m_treeView->setLevels( QList<int>() << CategoryId::Artist << CategoryId::Album );
}

void
CollectionWidget::sortByArtistYearAlbum()
{
    m_treeView->setShowYears( true );
    m_treeView->setLevels( QList<int>() << CategoryId::Artist << CategoryId::Album );
}

void
CollectionWidget::sortByAlbum()
{
    m_treeView->setShowYears( false );
    m_treeView->setLevels( QList<int>() << CategoryId::Album );
}

void
CollectionWidget::sortByGenreArtist()
{
    m_treeView->setShowYears( false );
    m_treeView->setLevels( QList<int>() << CategoryId::Genre << CategoryId::Artist );
}

void
CollectionWidget::sortByGenreArtistAlbum()
{
    m_treeView->setShowYears( false );
    m_treeView->setLevels( QList<int>() << CategoryId::Genre << CategoryId::Artist << CategoryId::Album );
}


#include "CollectionWidget.moc"

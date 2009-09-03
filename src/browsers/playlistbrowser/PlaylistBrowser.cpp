/****************************************************************************************
 * Copyright (c) 2007 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "PlaylistBrowser.h"

#include "Amarok.h"
#include "Debug.h"
#include "DynamicCategory.h"
#include "Playlist.h"
#include "PlaylistCategory.h"
#include "PodcastMeta.h"
#include "PodcastModel.h"
#include "PodcastCategory.h"
#include "PlaylistManager.h"

#include <kicon.h>
#include <klocale.h>
#include <KStandardDirs>

#include <QToolBox>
#include <QTreeView>

namespace PlaylistBrowserNS {

PlaylistBrowser::PlaylistBrowser( const char *name, QWidget *parent )
 : BrowserCategoryList( parent, name )
{
    DEBUG_BLOCK

    //setStyleSheet("QToolBox::tab { border-radius: 5px; border-color: red; border-style: solid }");

    setObjectName( name );

    //m_toolBox->setStyleSheet( "{}" );

    setMargin( 0 );
    setContentsMargins(0,0,0,0);

    addCategory( PlaylistManager::Dynamic );

    QList<int> categories = The::playlistManager()->availableCategories();
    debug() << categories.size() << " categories available";
    foreach( int category, categories )
    {
        debug() << "adding category nr. " << category;
        addCategory( category );
    }

    showCategory( Amarok::config( "Playlist Browser" ).readEntry( "Current Category", 1 ) );

    connect( The::playlistManager(), SIGNAL( categoryAdded( int ) ), SLOT( addCategory( int ) ) );
    connect( The::playlistManager(), SIGNAL( showCategory( int ) ), SLOT( showCategory( int ) ) );

    setLongDescription( i18n( "The playlist browser contains your list of imported and saved playlists. It is also where you can specify powerful dynamic playlists and manage your podcast  subscriptions and episodes." ) );

    setImagePath( KStandardDirs::locate( "data", "amarok/images/hover_info_playlists.png" ) );
}

PlaylistBrowser::~PlaylistBrowser()
{
    //Amarok::config( "Playlist Browser" ).writeEntry( "Current Category", m_toolBox->currentIndex() );
}

//SLOT
void
PlaylistBrowser::addCategory( int category )
{
    DEBUG_BLOCK
    QString categoryName = The::playlistManager()->categoryName( category );
    BrowserCategory *bCategory = 0;

    KIcon icon = The::playlistManager()->categoryIcon( category );

    switch( category )
    {
        //we don't show the current playlist in the PlaylistBrowser (yet)
        case PlaylistManager::CurrentPlaylist: return;
        //TODO: add the UserPlaylistCategory widget
        case PlaylistManager::UserPlaylist: bCategory = new PlaylistCategory( 0 ); break;
        case PlaylistManager::PodcastChannel: bCategory = loadPodcastCategory(); break;
        case PlaylistManager::Dynamic: bCategory = loadDynamicCategory(); break;
        //TODO: add the SmartPlaylistCategory widget
        //case PlaylistManager::SmartPlaylist: bCategory = loadSmartPlaylistCategory(); break;
        //This must be a custom category
        default: break;//TODO: widget = loadCustomCategory( int category );
    }

    BrowserCategoryList::addCategory( bCategory );

}

BrowserCategory *
PlaylistBrowser::loadPodcastCategory()
{
    return The::podcastCategory();
}

BrowserCategory*
PlaylistBrowser::loadDynamicCategory()
{
    return new DynamicCategory( 0 );
}

void
PlaylistBrowser::showCategory( int category )
{
    Q_UNUSED( category )
    DEBUG_BLOCK;
    //m_toolBox->setCurrentIndex( m_categoryIndexMap.key( category ) );
}

int PlaylistBrowserNS::PlaylistBrowser::currentCategory()
{
    //return m_categoryIndexMap.value( m_toolBox->currentIndex() );
    return 0;
}

}

#include "PlaylistBrowser.moc"

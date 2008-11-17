/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

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

#include <QHeaderView>
#include <QToolBox>
#include <QTreeView>

namespace PlaylistBrowserNS {

PlaylistBrowser::PlaylistBrowser( const char *name, QWidget *parent )
 : KVBox(parent)
{
    DEBUG_BLOCK

    //setStyleSheet("QToolBox::tab { border-radius: 5px; border-color: red; border-style: solid }");

    setObjectName( name );
    m_toolBox = new QToolBox( this );

    //m_toolBox->setStyleSheet( "{}" );

    setMargin( 0 );
    setContentsMargins(0,0,0,0);
    setFrameShape( QFrame::NoFrame );
    
    addCategory( PlaylistManager::Dynamic );

    QList<int> categories = The::playlistManager()->availableCategories();
    debug() << categories.size() << " categories available";
    foreach( int category, categories )
    {
        debug() << "adding category nr. " << category;
        addCategory( category );
    }


    connect( The::playlistManager(), SIGNAL( categoryAdded( int ) ), SLOT( addCategory( int ) ) );
    connect( The::playlistManager(), SIGNAL( showCategory( int ) ), SLOT( showCategory( int ) ) );
}

PlaylistBrowser::~PlaylistBrowser()
{
}

//SLOT
void
PlaylistBrowser::addCategory( int category )
{
    DEBUG_BLOCK

    QString typeName = The::playlistManager()->typeName( category );

    QWidget *widget = 0;

    //TODO: PlaylistBrowser::iconForCategory( int playlistCategory )
    KIcon icon = KIcon( "view-media-playlist-amarok" );

    switch( category )
    {
        //we don't show the current playlist in the PlaylistBrowser (yet)
        case PlaylistManager::CurrentPlaylist: return;
        //TODO: add the UserPlaylistCategory widget
        case PlaylistManager::UserPlaylist: widget = new PlaylistCategory( m_toolBox ); break;
        case PlaylistManager::PodcastChannel: widget = loadPodcastCategory(); break;
        case PlaylistManager::Dynamic: widget = loadDynamicCategory(); break;
        //TODO: add the SmartPlaylistCategory widget
        case PlaylistManager::SmartPlaylist: widget = new QTreeView( m_toolBox ); break;
        //This must be a custom category
        default: break;//TODO: widget = loadCustomCategory( int category );
    }

    m_toolBox->addItem( widget, icon, typeName );
}

QWidget *
PlaylistBrowser::loadPodcastCategory()
{
    PodcastModel *podcastModel = new PodcastModel();
    return new PodcastCategory( podcastModel );
}


QWidget*
PlaylistBrowser::loadDynamicCategory()
{
    return new DynamicCategory( m_toolBox );
}

void
PlaylistBrowser::showCategory( int category )
{
    DEBUG_BLOCK;
    m_toolBox->setCurrentIndex( category );
}

}

#include "PlaylistBrowser.moc"


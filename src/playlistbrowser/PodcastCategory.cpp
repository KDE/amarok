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

#include "amarok.h"

#include "PodcastCategory.h"
#include "PodcastModel.h"

#include <QAction>
#include <QToolBar>
#include <QHeaderView>
#include <qnamespace.h>

namespace PlaylistBrowserNS {

PodcastCategory::PodcastCategory( PlaylistBrowserNS::PodcastModel *podcastModel )
 : Ui_PodcastCategoryBase()
    , m_podcastModel( podcastModel )
{
    Ui_PodcastCategoryBase::setupUi( this );
    podcastTreeView->setModel( podcastModel );
    podcastTreeView->header()->hide();

    addPodcastButton->setIcon( KIcon( Amarok::icon( "add_playlist" ) ) );
    refreshPodcastsButton->setIcon( KIcon( Amarok::icon( "refresh" ) ) );
    configurePodcastsButton->setIcon( KIcon( Amarok::icon( "configure" ) ) );
    podcastsIntervalButton->setIcon( KIcon( Amarok::icon( "configure" ) ) );

    connect( addPodcastButton, SIGNAL( pressed() ), m_podcastModel, SLOT( addPodcast() ) );
    connect( refreshPodcastsButton, SIGNAL( pressed() ), m_podcastModel, SLOT( refreshPodcasts() ) );
    connect( configurePodcastsButton, SIGNAL( pressed() ), m_podcastModel, SLOT( configurePodcasts() ) );
    connect( podcastsIntervalButton, SIGNAL( pressed() ), m_podcastModel, SLOT( setPodcastsInterval() ) );
}


PodcastCategory::~PodcastCategory()
{
}

}

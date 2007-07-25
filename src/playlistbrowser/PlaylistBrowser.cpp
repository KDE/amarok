/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@gmail.com>

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
#include "Playlist.h"
#include "PodcastMeta.h"

#include <QDirModel>
#include <QHeaderView>
#include <QToolBox>
#include <QTreeView>

namespace PlaylistBrowserNS {

PlaylistBrowser::PlaylistBrowser( const char *name )
 : KVBox()
{
    QToolBox *toolBox = new QToolBox();

    QDirModel *model = new QDirModel;
    QTreeView *tree = new QTreeView(toolBox);
    tree->setModel(model);
    tree->setRootIndex(model->index(QDir::currentPath()));
    tree->header()->hide();

    toolBox->addItem( tree, QString( "Podcasts" ) );
}


PlaylistBrowser::~PlaylistBrowser()
{
}


}

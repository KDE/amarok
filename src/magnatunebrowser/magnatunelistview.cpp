/*
  Copyright (c) 2006  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "magnatunedatabasehandler.h"
#include "magnatunelistview.h"

#include <klocale.h>

MagnatuneListView::MagnatuneListView(QWidget * parent)
 : QListView(parent)
{

    setRootIsDecorated(true);
    addColumn( i18n( "Artist/Album/Track" ) );
    addColumn( i18n( "Duration" ) );
    
}

MagnatuneListView::~MagnatuneListView()
{}

KURLDrag * MagnatuneListView::dragObject( )
{
    KURL::List urls;
    int id;
    MagnatuneTrackList tracks;
    MagnatuneTrackList::iterator it;

    QListViewItem * pSelectedItem = selectedItem();

    switch (pSelectedItem->depth()) {
        case 0:
            id = ((MagnatuneListViewTrackItem *) pSelectedItem)->getId();
            tracks = MagnatuneDatabaseHandler::instance()->getTracksByArtistId(id);
    	    for ( it = tracks.begin(); it != tracks.end(); ++it ) {
		urls.append((*it).getHifiURL());	
            }
            break;
        case 1:
            id = ((MagnatuneListViewTrackItem *) pSelectedItem)->getId();
            tracks = MagnatuneDatabaseHandler::instance()->getTracksByAlbumId(id);
    	    for ( it = tracks.begin(); it != tracks.end(); ++it ) {
		urls.append((*it).getHifiURL());	
            }
            break;
        case 2:
           urls.append(((MagnatuneListViewTrackItem *) pSelectedItem)->getHifiURL( ));
           break;
   }

    KURLDrag* d = new KURLDrag( urls, this );
    return d;
}



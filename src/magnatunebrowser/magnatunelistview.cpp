//
// C++ Implementation: magnatunelistview
//
// Description: 
//
//
// Author: Mark Kretschmann <markey@web.de>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "magnatunelistview.h"
#include "magnatunedatabasehandler.h"

MagnatuneListView::MagnatuneListView(QWidget * parent)
 : QListView(parent)
{}

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



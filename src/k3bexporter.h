/****************************************************************************************
 * Copyright (c) 2004 Michael Pyne <michael.pyne@kdemail.net>                           *
 * Copyright (c) 2004 Pierpaolo Di Panfilo <pippo_dp@libero.it>                         *
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

#ifndef K3BEXPORTER_H
#define K3BEXPORTER_H

#include "amarok_export.h"

#include <KUrl>

//class DCOPRef;

/**
 * This class will export a list of tracks to K3b.
 */
class K3bExporter
{
public:
    enum K3bOpenMode { AudioCD, DataCD, Abort };

    /**
    * @return true if the executable of K3B is found
    */
    // FIXME: implement me!
    /*AMAROK_EXPORT*/ static bool isAvailable() { return false; }


    /**
    * Exports the list of urls @urls via DCOP to K3B. The mode @p openmode will be used
    * @param urls The list of urls to export
    * @param openmode The mode of the album
    */
    // FIXME: implement me!
    /*AMAROK_EXPORT*/ void exportTracks( const KUrl::List &/*urls*/, int openmode=-1 ) { Q_UNUSED(openmode) }

    /**
    * Exports the current playlist to K3B. The mode @p openmode will be used
    * @param openmode The mode of the album
    */
    // FIXME: implement me!
    void exportCurrentPlaylist( int openmode=-1 ) { Q_UNUSED(openmode) }

    /**
    * Exports the selected tracks to K3B. The mode @p openmode will be used
    * @param openmode The mode of the tracks
    */
    // FIXME: implement me!
    void exportSelectedTracks( int openmode=-1 ) { Q_UNUSED(openmode) }

    /**
    * Exports the album @p album to K3B. The mode @p openmode will be used
    * @param openmode The mode of the album
    * @param album The album to export
    */
    // FIXME: implement me!
    void exportAlbum( const QString &/*album*/, int openmode=-1 ) { Q_UNUSED(openmode) }

    /**
    * Exports the album @p album by artist @ artist to K3B. The mode @p openmode will be used
    * @param openmode The mode of the album
    * @param album The album to export
    */
    // FIXME: implement me!
    void exportAlbum( const QString &/*artist*/, const QString &/*album*/, int openmode=-1 ) { Q_UNUSED(openmode) }

    /**
    * Exports all tracks of the artist @p artist to K3B. The mode @p openmode will be used
    * @param openmode The mode of the album
    * @param artist The artists which tracks to export
    */
    // FIXME: implement me!
    void exportArtist( const QString &/*artist*/, int openmode=-1 ) { Q_UNUSED(openmode) }

    /**
    * Exports all tracks of the composer @p composer to K3B. The mode @p openmode will be used
    * @param openmode The mode of the album
    * @param artist The artists which tracks to export
    */
    // FIXME: implement me!
    void exportComposer( const QString &/*artist*/, int openmode=-1 ) { Q_UNUSED(openmode) }

    /**
    * @return the static instance of K3bExporter
    */
    static K3bExporter *instance() { return s_instance; }

private:
    /*void exportViaCmdLine( const KUrl::List &urls, int openmode );
    void exportViaDCOP( const KUrl::List &urls, DCOPRef &ref, int mode );
    void DCOPErrorMessage();
    bool startNewK3bProject( DCOPRef &ref, int mode );
    K3bOpenMode openMode();*/

    AMAROK_EXPORT static K3bExporter *s_instance;
};


#endif /* K3BEXPORTER_H */

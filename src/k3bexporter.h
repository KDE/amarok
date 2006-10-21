/***************************************************************************
    begin                : Mon May 31 2004
    copyright            : (C) 2004 by Michael Pyne
                           (c) 2004 by Pierpaolo Di Panfilo
    email                : michael.pyne@kdemail.net
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3BEXPORTER_H
#define K3BEXPORTER_H

#include "amarok_export.h"
#include "collectiondb.h"
#include <kurl.h>

class DCOPRef;

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
    LIBAMAROK_EXPORT static bool isAvailable();


    /**
    * Exports the list of urls @urls via DCOP to K3B. The mode @p openmode will be used
    * @param urls The list of urls to export
    * @param openmode The mode of the album
    */
    LIBAMAROK_EXPORT void exportTracks( const KURL::List &urls, int openmode=-1 );

    /**
    * Exports the current playlist to K3B. The mode @p openmode will be used
    * @param openmode The mode of the album
    */
    void exportCurrentPlaylist( int openmode=-1 );

    /**
    * Exports the selected tracks to K3B. The mode @p openmode will be used
    * @param openmode The mode of the tracks
    */
    void exportSelectedTracks( int openmode=-1 );

    /**
    * Exports the album @p album to K3B. The mode @p openmode will be used
    * @param openmode The mode of the album
    * @param album The album to export
    */
    void exportAlbum( const QString &album, int openmode=-1 );

    /**
    * Exports the album @p album by artist @ artist to K3B. The mode @p openmode will be used
    * @param openmode The mode of the album
    * @param album The album to export
    */
    void exportAlbum( const QString &artist, const QString &album, int openmode=-1 );

    /**
    * Exports all tracks of the artist @p artist to K3B. The mode @p openmode will be used
    * @param openmode The mode of the album
    * @param artist The artists which tracks to export
    */
    void exportArtist( const QString &artist, int openmode=-1 );

    /**
    * Exports all tracks of the composer @p composer to K3B. The mode @p openmode will be used
    * @param openmode The mode of the album
    * @param artist The artists which tracks to export
    */
    void exportComposer( const QString &artist, int openmode=-1 );

    /**
    * @return the static instance of K3bExporter
    */
    static K3bExporter *instance() { return s_instance; }

private:
    void exportViaCmdLine( const KURL::List &urls, int openmode );
    void exportViaDCOP( const KURL::List &urls, DCOPRef &ref, int mode );
    void DCOPErrorMessage();
    bool startNewK3bProject( DCOPRef &ref, int mode );
    K3bOpenMode openMode();

    LIBAMAROK_EXPORT static K3bExporter *s_instance;
};


#endif /* K3BEXPORTER_H */

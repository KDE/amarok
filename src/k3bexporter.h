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


#include <kurl.h>

class DCOPRef;

/**
 * This class will export a list of tracks to K3b.
 */
class K3bExporter
{
public:
    enum K3bOpenMode { AudioCD, DataCD, Abort };

    K3bExporter();
    ~K3bExporter() {}
    static bool isAvailable();
    void exportTracks( const KURL::List &urls, int openmode=-1 );

    void exportCurrentPlaylist( int openmode=-1 );    //export current playlist to k3b
    void exportSelectedTracks( int openmode=-1 );    //export selected tracks in playlist to k3b

    static K3bExporter *instance() { return s_instance; }

private:
    void exportViaCmdLine( const KURL::List &urls, int openmode );
    void exportViaDCOP( const KURL::List &urls, DCOPRef &ref, int mode );
    void DCOPErrorMessage();
    bool startNewK3bProject( DCOPRef &ref, int mode );
    K3bOpenMode openMode();

    static K3bExporter *s_instance;
};


#endif /* K3BEXPORTER_H */

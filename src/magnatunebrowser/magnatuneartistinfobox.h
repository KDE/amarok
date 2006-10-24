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

#ifndef MAGNATUNEARTISTINFOBOX_H
#define MAGNATUNEARTISTINFOBOX_H

#include "amarok.h"
#include "magnatunetypes.h"
#include "statusbar.h"

#include <khtml_part.h>
#include <kio/jobclasses.h>
#include <kio/job.h>

/**
A specialized KHTMLPart for displaying html info about a Magnatune artist or album
 
@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class MagnatuneArtistInfoBox : public KHTMLPart
{
    Q_OBJECT

public:
    /**
     * Constructor
     * @param parentWidget The parent QWidget
     * @param widgetname The name of this widget
     * @return New MagnatuneArtistInfoBox object
     */
    MagnatuneArtistInfoBox( QWidget *parentWidget, const char *widgetname );

    /**
     * Destructor
     * @return Nothing
     */
    ~MagnatuneArtistInfoBox();

    /**
     * Fetches Magnatune artist info from the url and formats
     * it for display. 
     * @param url The url of the Magnatune artist page
     * @return currently always returns true
     */
    bool displayArtistInfo( KURL url );

    /**
     * Display info about a Magnatune album. Retrieves cover url and 
     * other info from the album.
     * @param album The album to display info about.
     * @return Always returns true at the moment
     */
    bool displayAlbumInfo( MagnatuneAlbum *album );

protected:

    KIO::TransferJob *m_infoDownloadJob;

    /**
     * Helper function for extracting only the part of the artist page 
     * that we need. Used by displayArtistInfo
     * @param artistPage The artist url
     * @return A string containing the artist info as html.
     */
    QString extractArtistInfo( QString artistPage );

    /**
     * Helper method to reset the scrollbars when loading a new album
     * or artist info page.
     */
    void resetScrollBars();


protected slots:

    /**
     * Slot for recieving notifications from the download KIO::Job
     * @param downLoadJob The job that has completed
     */
    void infoDownloadComplete( KIO::Job *downLoadJob );

};

#endif

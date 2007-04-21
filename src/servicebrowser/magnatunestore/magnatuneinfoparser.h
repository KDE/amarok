/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#ifndef MAGNATUNEAINFOPARSER_H
#define MAGNATUNEAINFOPARSER_H

#include "amarok.h"
#include "magnatunetypes.h"
#include "statusbar.h"

#include <kio/jobclasses.h>
#include <kio/job.h>

/**
A specialized KHTMLPart for displaying html info about a Magnatune artist or album
 
@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class MagnatuneInfoParser : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructor
     */
    MagnatuneInfoParser(  );

    /**
     * Destructor
     * @return Nothing
     */
    ~MagnatuneInfoParser();

    /**
     * Fetches info about artist and emits infoReady( Qstring ) 
     * with a ready to show html page when the info is ready
     * @param artist The artist to get info about
     */
    void getInfo( MagnatuneArtist *artist );

    /**
     * Overloaded function
     * Fetches info about album and emits infoReady( Qstring ) 
     * with a ready to show html page when the info is ready
     * @param url The album to get info about
     */
    void getInfo( MagnatuneAlbum *album );

signals:

    void info( QString );

private:
      
    KIO::StoredTransferJob *m_infoDownloadJob;

    QString extractArtistInfo( const QString &artistPage );

protected slots:

    /**
     * Slot for recieving notifications from the download KIO::Job
     * @param downLoadJob The job that has completed
     */
    void artistInfoDownloadComplete( KJob *downLoadJob);

};

#endif

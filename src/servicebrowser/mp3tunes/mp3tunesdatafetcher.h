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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#ifndef MP3TUNESDATAFETCHER_H
#define MP3TUNESDATAFETCHER_H

#include "../databasehandlerbase.h"

#include <QObject>

#include <kio/jobclasses.h>
#include <kio/job.h>

/**
An experimental class for fetching data from mp3tunes.com

	@author 
*/
class Mp3tunesDataFetcher : public QObject, public DatabaseHandlerBase
{
Q_OBJECT
public:
    Mp3tunesDataFetcher();

    ~Mp3tunesDataFetcher();

    /**
     * Validates user and retrieves session id
     * @param username the username
     * @param password the password
     * @return the session id
     */
    int authenticate( const QString &username = QString(), const QString &password = QString() );



    virtual void createDatabase() {}
    virtual void destroyDatabase() {}
    virtual int getArtistIdByExactName(const QString &name);
    virtual SimpleServiceArtistList getArtistsByGenre( const QString &genre );
    virtual SimpleServiceArtist * getArtistById( int id );
    virtual SimpleServiceAlbum * getAlbumById( int id );
    virtual SimpleServiceTrack * getTrackById( int id );
    virtual SimpleServiceAlbumList getAlbumsByArtistId(int id, const QString &genre);
    virtual SimpleServiceTrackList getTracksByAlbumId(int id);
    virtual QStringList getAlbumGenres();


private:

    QString m_apiOutputFormat;
    QString m_partnerToken;
    QString m_sessionId;

    KIO::StoredTransferJob *m_xmlDownloadJob;




private slots:

    void authenticationComplete(  KJob *job );
    void getArtistsComplete(  KJob *job );
    void getAlbumsComplete(  KJob *job );
    void getTracksComplete(  KJob *job );

};

#endif

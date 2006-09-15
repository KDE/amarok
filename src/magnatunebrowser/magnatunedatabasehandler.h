//
// C++ Interface: magnatunedatabasehandler
//
// Description: 
//
//
// Author: Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef MAGNATUNEDATABASEHANDLER_H
#define MAGNATUNEDATABASEHANDLER_H

#include "magnatunetypes.h"
#include "collectiondb.h"

#include <qstring.h>
#include <qstringlist.h>



/**
This class wraps the database operations needed by the MagnatuneBrowser

	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class MagnatuneDatabaseHandler{
public:
    

    static MagnatuneDatabaseHandler * instance();

    ~MagnatuneDatabaseHandler();

    void createDatabase();
    void destroyDatabase();

    int insertTrack(MagnatuneTrack * track, int albumId, int artistId);
    int insertTracks(MagnatuneTrackList tracks, int artistId, int albumId);
    int insertAlbum(MagnatuneAlbum * album, int artistId);
   
    int insertArtist(MagnatuneArtist * artist);

    //get id, or -1 if artist does not exist
    int GetArtistIdByExactName(QString name);

    MagnatuneArtistList getArtistsByGenre(QString genre);
    MagnatuneArtist 	getArtistById(int id);
    MagnatuneAlbumList  getAlbumsByArtistId(int id, QString genre);
    MagnatuneTrackList  getTracksByAlbumId(int id);
    MagnatuneTrackList  getTracksByArtistId(int id);  //used for adding all tracks by a given artist to playlist

    QStringList getAlbumGenres();

    void begin();
    void commit();


protected:

   MagnatuneDatabaseHandler();
   static MagnatuneDatabaseHandler * m_pInstance;

};

#endif

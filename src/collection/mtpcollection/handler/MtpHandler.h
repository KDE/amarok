/***************************************************************************
 * Ported to Collection Framework: *
 * copyright            : (C) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com> 

 * Original Work: *
 * copyright            : (C) 2006 Andy Kelk <andy@mopoke.co.uk>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy 
 * defined in Section 14 of version 3 of the license.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#ifndef MTPHANDLER_H
#define MTPHANDLER_H

#include <libmtp.h>

#include "Meta.h"
#include "MemoryCollection.h"
#include "MtpMeta.h"

#include "kjob.h"

#include <QObject>
#include <QMap>
#include <QMutex>

class QString;
class QFile;
class QDateTime;
class QMutex;
class QStringList;

class MtpCollection;

namespace Mtp
{

/* The libmtp backend for all Mtp calls */
    class MtpHandler : public QObject
    {
        
        Q_OBJECT

        public:
            MtpHandler( MtpCollection *mc, QObject* parent );
           ~MtpHandler();

          

       // external functions
       void copyTrackToDevice( const Meta::TrackPtr &track );
       bool deleteTrackFromDevice( const Meta::MtpTrackPtr &track );
       void parseTracks();
       QString prettyName() const;

       // file io functions
       bool kioCopyTrack( const KUrl &src, const KUrl &dst );
       void deleteFile( const KUrl &url );

       // internal mtp functions

       int                     readMtpMusic( void );
       void getBasicMtpTrackInfo( LIBMTP_track_t *mtptrack, Meta::MtpTrackPtr track );
       
       
       // miscellaneous internal functions
       void addMtpTrackToCollection( LIBMTP_track_t *mtptrack );

       // convenience methods to avoid repetitive code

       void setupArtistMap( LIBMTP_track_t *mtptrack, Meta::MtpTrackPtr track, ArtistMap &artistMap );
       void setupAlbumMap( LIBMTP_track_t *mtptrack, Meta::MtpTrackPtr track, AlbumMap &albumMap );
       void setupGenreMap( LIBMTP_track_t *mtptrack, Meta::MtpTrackPtr track, GenreMap &genreMap );
       void setupComposerMap( LIBMTP_track_t *mtptrack, Meta::MtpTrackPtr track, ComposerMap &composerMap );
       void setupYearMap( LIBMTP_track_t *mtptrack, Meta::MtpTrackPtr track, YearMap &yearMap );
        

      
           
        public slots:
	void fileTransferred( KJob *job );
    void fileDeleted( KJob *job );

        signals:

        private:
            MtpCollection *m_memColl;
	    
        // mtp database

        LIBMTP_mtpdevice_t      *m_device;

        QMap<int,QString>       mtpFileTypes;

        uint32_t                m_default_parent_folder;
        LIBMTP_folder_t        *m_folders;
        QString                 m_format;
        QString                 m_name;
        QStringList             m_supportedFiles;
        

        QMutex                  m_critical_mutex;

        bool             m_trackCreated;

        // KIO-related Vars (to be moved elsewhere eventually)

        bool m_copyFailed;
        bool m_isCanceled;
        bool m_wait;


    };
}
#endif

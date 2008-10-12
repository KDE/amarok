/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>
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

#include "PlaylistFileSupport.h"
#include "Debug.h"
#include "collection/CollectionManager.h"
#include "statusbar_ng/StatusBar.h"
#include "meta/XSPFPlaylist.h"
#include "meta/PLSPlaylist.h"
#include "meta/M3UPlaylist.h"


#include <KLocale>
#include <KTemporaryFile>
#include <KUrl>

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace Meta {

Meta::Format
getFormat( const QString &filename )
{
    //debug() << "filename: " << filename;
    const QString ext = Amarok::extension( filename );

    if( ext == "m3u" ) return M3U;
    if( ext == "pls" ) return PLS;
    if( ext == "ram" ) return RAM;
    if( ext == "smil") return SMIL;
    if( ext == "asx" || ext == "wax" ) return ASX;
    if( ext == "xml" ) return XML;
    if( ext == "xspf" ) return XSPF;

    return Unknown;
}

PlaylistPtr
loadPlaylist( const KUrl &url )
{
    DEBUG_BLOCK

    QFile file;
    PlaylistPtr playlist;

    if ( url.isLocalFile() ){

        debug() << "local file";

        file.setFileName( url.path() );

        if( !file.open( QIODevice::ReadOnly | QIODevice::Text ) )
        {
            debug() << "could not read file " << url.path();
            The::statusBarNG()->longMessageThreadSafe( i18n( "Cannot read playlist (%1).", url.url() ) );
            return playlist;
        }
    } else {

        debug() << "remote file: " << url;

        //FIXME: for now, just do a blocking download... Someone please come up with a better way...

        
        KTemporaryFile tempFile;

        tempFile.setSuffix(  '.' + Amarok::extension( url.url() ) );
        tempFile.setAutoRemove( false );  //file will be removed in JamendoXmlParser
        if( !tempFile.open() )
        {
            return playlist; //error
        }


        QString tempFileName = tempFile.fileName();
#ifdef Q_WS_WIN
        // KIO::file_copy faild to overwrite an open file 
        // using KTemporary.close() is not enough here
        tempFile.remove();
#endif
        KIO::FileCopyJob * job = KIO::file_copy( url , KUrl( tempFileName ), 0774 , KIO::Overwrite | KIO::HideProgressInfo );

        //FIXME!! Re-enable after end of string freeze
        //The::statusBarNG()->newProgressOperation( job, i18n( "Fetching remote playlist" ) );
        

        

        if ( !job->exec() )
        {
            debug() << "error";
            job->deleteLater();
            return playlist;
        }
        else
        {
            debug() << "gotcha: " << tempFileName;
            job->deleteLater();
            file.setFileName( tempFileName );
            file.open( QFile::ReadOnly );
        }

    }

    Format format = getFormat( file.fileName() );
    switch( format ) {
        case PLS:
            playlist = new PLSPlaylist( KUrl( QFileInfo(file).filePath()) );
            break;
        case M3U:
            playlist = new M3UPlaylist( KUrl( QFileInfo(file).filePath()) );
            break;
//         case RAM:
//             playlist = loadRealAudioRam( stream );
//             break;
//         case ASX:
//             playlist = loadASX( stream );
//             break;
//         case SMIL:
//             playlist = loadSMIL( stream );
//             break;
        case XSPF:
            playlist = new XSPFPlaylist( KUrl( QFileInfo(file).filePath()) );
            break;

        default:
            debug() << "unknown type!";
            break;
    }

    return playlist;

}

}

// (c) 2005 Martin Ehmke <ehmke@gmx.de>
// License: GNU General Public License V2

#define DEBUG_PREFIX "CueFile"

#include <qfile.h>
#include <qmap.h>
#include <qstringlist.h>

#include "cuefile.h"
#include "debug.h"


/**
* @return true if the cuefile could be successfully loaded
*/
bool CueFile::load()
{
    if( QFile::exists( m_cueFileName ) ) {
        clear();

        QFile file( m_cueFileName );
        int track = 0;
        QString artist = QString::null;
        QString title = QString::null;
        long index = 0;

        // for a valid cuefile at least TRACK and INDEX is required
        // mode = 0: beginning, mode = 1: track found, index not yet found, mode = 2: index already found
        int mode = 0;
        if( file.open( IO_ReadOnly ) )
        {
            QTextStream stream( &file );
            QString line;

            while ( !stream.atEnd() )
            {
                line = stream.readLine().simplifyWhiteSpace();

                if( line.startsWith( "title", false ) )
                {
                    if( mode )
                        title = line.mid( 6 ).remove( '"' );
                    kdDebug() << "[CUEFILE]: Title: " << title << endl;
                }

                else if( line.startsWith( "performer", false ))
                {
                    if( mode )
                        artist = line.mid( 10 ).remove( '"' );
                    kdDebug() << "[CUEFILE]: artist: " << artist << endl;
                }

                else if( line.startsWith( "track", false ) )
                {
                    if( mode == 1 )
                    {
                        // not valid, because we have to have an index for the previous track
                        file.close();
                        kdDebug() << "[CUEFILE]: Mode is 1, abort." << endl;
                        return false;
                    }
                    if( mode == 2 )
                    {
                        kdDebug() << "[CUEFILE]: Inserting item: " << title << " - " << artist << " (" << track << ")" << endl;
                        // add previous entry to map
                        insert( index, CueFileItem( title, artist, track, index ) );
                        index    = 0;
                        title  = QString::null;
                        artist = QString::null;
                        track    = 0;
                    }
                    track = line.section (' ',1,1).toInt();
                    kdDebug() << "[CUEFILE]: Track: " << track << endl;
                    mode = 1;
                }
                else if( line.startsWith( "index", false ) )
                {
                    if( mode ) {
                        QStringList time = QStringList::split( QChar(':'),line.section (' ',-1,-1) );

                        index = (time[0].toLong()*60 + time[1].toLong())*1000 + time[2].toLong()*1000/75; //75 frames per second
                        mode = 2;
                    }
                    kdDebug() << "[CUEFILE]: index: " << index << endl;
                }
            }
            kdDebug() << "[CUEFILE]: Inserting item: " << title << " - " << artist << " (" << track << ")" << endl;
            // add previous entry to map
            insert( index, CueFileItem( title, artist, track, index ) );
            file.close();
        }
        return true;
    }
    else {
        return false;
    }
}


#include "cuefile.moc"




/***************************************************************************
                          track.cpp  -  description
                             -------------------
    begin                : 2001-10-13
    copyright            : (C) 2001 by Shaun Jackman (sjackman@debian.org)
    modify by            : Andres Oton
    email                : andres.oton@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

static const char* rcsid __attribute__((unused)) =
"$Id: track.cpp,v 1.10.2.8 2005/07/05 00:25:09 acejones Exp $";


// qt
#include <qregexp.h>
#include <qptrlist.h>

// kde
#include <kdebug.h>

// libid3
// #include <id3/tag.h>
// #include <id3/misc_support.h>
// #include <id3/globals.h>

// kionjb
#include "track.h"
#include "njbmediadevice.h"


#ifdef HAVE_SQLITE
// sqlite
#include "sqlite.h"
#endif

/* ------------------------------------------------------------------------ */
NjbTrack::NjbTrack( char** result)
{
    id = atoi( result[ 0]);
    size = atoi( result[ 1]);
    duration = atoi( result[ 2]);
    tracknum = atoi( result[ 3]);
    genre = result[ 4];
    // Form genre : remove starting and ending parenthesis and convert to
    //	genre name. Ex : (35) -> "House"
    genre.remove(0, 1);
    genre.remove((int)(genre.length()-1), 1);
    // genre = ID3_v1_genre_description[genre.toInt()];
    artist = result[ 5];
    album = result[ 6];
    title = result[ 7];
    codec = result[ 8];
    filename = result[ 9];
    //FIXME support de year
}


/* ------------------------------------------------------------------------ */
NjbTrack::NjbTrack( njb_songid_t* song)
{
    // kdDebug( 7182) << __func__ << ": pid=" << getpid() << endl;

    njb_songid_frame_t* frame;

    id = song->trid;

    frame = NJB_Songid_Findframe( song, FR_SIZE);
     //	kdDebug( 7182) << "frame = NJB_Songid_Findframe( song, FR_SIZE);" << endl;
    if ( frame->type == NJB_TYPE_UINT32 )
        size = frame->data.u_int32_val;
    else
    {
        size = 0;
        kdError( 7182) << __func__ << " Unexpected frame type:" << frame->type << endl;
    }

    frame = NJB_Songid_Findframe( song, FR_LENGTH);
    // kdDebug( 7182) << "frame = NJB_Songid_Findframe( song, FR_LENGTH);" << endl;
    if ( frame->type == NJB_TYPE_UINT16 )
        duration = frame->data.u_int16_val;
    else
    {
        duration = 0;
        kdError( 7182) << __func__ << " Unexpected frame type:" << frame->type << endl;
    }

    frame = NJB_Songid_Findframe( song, FR_GENRE);
    // kdDebug( 7182) << "frame = NJB_Songid_Findframe( song, FR_GENRE);" << endl;
    if( frame) {
        genre = QCString( frame->data.strval);
        // Form genre : remove starting and ending parenthesis and convert to 
        //	genre name	(ex : (35) -> "House")
        //if( genre.startsWith( "(") && genre.endsWith( ")")) {
        //	genre.remove(0, 1);
        //	genre.remove((int)(genre.length()-1), 1);
        //	genre = ID3_v1_genre_description[genre.toInt()];
        //}
    } else
        genre = "(none)";

    frame = NJB_Songid_Findframe( song, FR_ARTIST);
    // kdDebug( 7182) << "frame = NJB_Songid_Findframe( song, FR_ARTIST);" << endl;
    if( frame) {
        artist = QCString( frame->data.strval);
        artist.replace( QRegExp( "/"), "-");
    }

    frame = NJB_Songid_Findframe( song, FR_ALBUM);
    // kdDebug( 7182) << "frame = NJB_Songid_Findframe( song, FR_ALBUM);" << endl;
    if( frame) {
        album = QCString( frame->data.strval);
        album.replace( QRegExp( "/"), "-");
    } else
        album = "<Unknown>";

    frame = NJB_Songid_Findframe( song, FR_TITLE);
    // kdDebug( 7182) << "frame = NJB_Songid_Findframe( song, FR_TITLE);" << endl;
    if( frame) {
        title = QCString( frame->data.strval);
        title.replace( QRegExp( "/"), "-");
    }

    frame = NJB_Songid_Findframe( song, FR_TRACK);
    // kdDebug( 7182) << "frame = NJB_Songid_Findframe( song, FR_TRACK);" << endl;
    if( frame) 
    {
        switch ( frame->type )
        {
        case NJB_TYPE_UINT16:
            tracknum = frame->data.u_int16_val;
            break;
        case NJB_TYPE_UINT32:
            tracknum = frame->data.u_int32_val;
            break;
        case NJB_TYPE_STRING:
            tracknum = QString(frame->data.strval).toUInt();
            break;
        default:
            tracknum = 0;
     // kdDebug( 7182) << __func__ << ": unknown data type returnd for FR_TRACK;" << endl;
        }
    }

    frame = NJB_Songid_Findframe( song, FR_CODEC);
    if( frame)
        codec = QCString( frame->data.strval).lower();
    else
        codec = "mp3";

    // kdDebug( 7182) << "frame = NJB_Songid_Findframe( song, FR_CODEC): " << codec << endl;

    frame = NJB_Songid_Findframe( song, FR_FNAME);
    if( frame)
        filename = QCString(  frame->data.strval);
    else
        filename = artist + " - " + title + "." + codec;
    // kdDebug( 7182) << "frame = NJB_Songid_Findframe( song, FR_FNAME): " << filename << endl;

    frame = NJB_Songid_Findframe( song, FR_YEAR);
    // kdDebug( 7182) << "frame = NJB_Songid_Findframe( song, FR_YEAR);" << endl;
    if( frame) 
    {
        switch ( frame->type )
        {
        case NJB_TYPE_UINT16:
            year = QString::number(frame->data.u_int16_val).utf8();
            break;
        case NJB_TYPE_UINT32:
            year = QString::number(frame->data.u_int32_val).utf8();
            break;
        case NJB_TYPE_STRING:
            year = QCString(frame->data.strval);
            break;
        default:
            year = "0";
    // kdDebug( 7182) << __func__ << ": unknown data type returnd for FR_YEAR;" << endl;
        }
    }
    // kdDebug( 7182) << __func__ << ": OK" << endl;
}


/* ------------------------------------------------------------------------ */
trackValueList::iterator 
trackValueList::findTrackByName( const QString& _filename )
{
    trackValueList::iterator it;
    for( it = begin(); it != end(); it++)
        if( (*it).getFilename() == _filename) 
            break;
    return it;
}

/* ------------------------------------------------------------------------ */
trackValueList::const_iterator 
trackValueList::findTrackByName( const QString& _filename ) const
{
    trackValueList::const_iterator it;
    for( it = begin(); it != end(); it++)
        if( (*it).getFilename() == _filename) 
            break;
    return it;
}

/* ------------------------------------------------------------------------ */
trackValueList::iterator 
trackValueList::findTrackById( unsigned _id )
{
    trackValueList::iterator it;
    for( it = begin(); it != end(); it++)
        if( (*it).getId() == _id) 
            break;
    return it;
}

/* ------------------------------------------------------------------------ */
trackValueList::const_iterator 
trackValueList::findTrackById( unsigned _id ) const
{
    trackValueList::const_iterator it;
    for( it = begin(); it != end(); it++)
        if( (*it).getId() == _id) 
            break;
    return it;
}

/* ------------------------------------------------------------------------ */
/** Transfer musical info from the njb to local structures */
int
trackValueList::readFromDevice( void)
{
    kdDebug( 7182) << __func__ << ": pid=" << getpid() << endl;

    // ONLY read in from the device if this list is empty. 
    // 
    // Support for cache:
    // - If the sqlite cache exists, and it's up-to-date with the device:
    //     Read from the cache instead of the device
#ifdef HAVE_SQLITE

    // if the libcounter is 0, we need to reread the cache no matter what
    unsigned long cachecounter = 0;
    char* errmsg;
    char** result;
    int nrow, ncolumn;
    sqlite* m_db;

    unsigned long counter = NJB_Get_NJB1_Libcounter( theNjb);
    if ( !counter )
        goto nocache;

    // Open up the tracks db, if it's there
    m_db = sqlite_open("tracks.db",0,&errmsg);
    if ( errmsg )
    {
        kdDebug(7182) << __func__ << ": cache file not used, " << errmsg << endl;
        free(errmsg);
        goto nocache;
    }

    // Fetch the cache counter from the file
    sqlite_get_table( m_db, "SELECT value from etc WHERE key == 'counter'", &result, &nrow, &ncolumn, &errmsg );
    if ( errmsg )
    {
        sqlite_free_table(result);
        kdDebug(7182) << __func__ << ": cache file not used, " << errmsg << endl;
        goto nocache;
    }

    // Compare the cache counter versus the device counter
    if ( nrow )
        cachecounter = QString(result[1]).toULong();
    if ( counter != cachecounter )
    {
        kdDebug(7182) << __func__ << ": cache file not used, counter=" << counter << ", cache=" << cachecounter << endl;
        goto nocache;
    }

    // Load from the cache

nocache:
#endif	
    // Load from the device
    int i = 0;
    // Don't get extended metadatas
    kdDebug(7182) << __func__ << ": theNjb is:" << theNjb << endl;
    NJB_Get_Extended_Tags(theNjb, 0);
    NJB_Reset_Get_Track_Tag( theNjb);
    while( njb_songid_t* song = NJB_Get_Track_Tag( theNjb)) {
        //FIXME (acejones) Make this a signal
        // 		infoMessage( i18n( "Downloading track %1...").arg( i++));
        append( NjbTrack(song));
        NJB_Songid_Destroy( song);
        ++i;
    }
    kdDebug( 7182) << __func__ << ": " << i << " jukebox tracks loaded from device." << endl;

#ifdef HAVE_SQLITE
    // Support for cache:
    // - After reading from device, wipe and rebuild cache

    // Open up the tracks db, if it's there
    m_db = sqlite_open("tracks.db",0,&errmsg);
    if ( errmsg )
    {
        kdDebug(7182) << __func__ << ": Unable to create db file, " << errmsg << endl;
        free(errmsg);
        goto done;
    }

done:
#endif
    return NJB_SUCCESS;
}

MetaBundle* NjbTrack::getMetaBundle( )
{
    MetaBundle *bundle = new MetaBundle();
    // 	unsigned id;
    // 	unsigned size;
    // 	unsigned duration;
    // 	unsigned tracknum;
    // 	QString year;
    // 	QString genre;
    // 	QString artist;
    // 	QString album;
    // 	QString title;
    // 	QString codec;
    // 	QString filename;

    bundle->setAlbum( album);
    bundle->setArtist( artist);
    bundle->setFilesize( size);
    bundle->setLength( duration);
    bundle->setTrack( tracknum);
    bundle->setYear( year.toInt());
    bundle->setGenre( genre);
    bundle->setTitle( title);
    //	bundle->setPath( filename );
    return bundle;
}

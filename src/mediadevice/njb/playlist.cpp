/***************************************************************************
                          playlist.cpp  -  description
                             -------------------
    begin                : 2001-07-24
    copyright            : (C) 2001 by Shaun Jackman (sjackman@debian.org)
    modify by:           : Andres Oton 
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


#include "debug.h"


// kionjb
#include "njbmediadevice.h"
#include "playlist.h"

#define NJB_SUCCESS 0
#define NJB_FAILURE -1



// KDE
#include <kdebug.h>
using namespace KIO;

// POSIX
#include <stdlib.h>

NjbPlaylist::NjbPlaylist()
{
    m_playlist = NJB_Playlist_New();
    if( !m_playlist)
        debug() << "putPlaylist: playlist_new failed\n";
}

NjbPlaylist::NjbPlaylist(njb_playlist_t* playlist):
    m_playlist(0)
{
    // 	debug() << __PRETTY_FUNCTION__ << " this=" << this << " playlist=" << playlist << endl;
    setPlaylist( playlist );
}

NjbPlaylist::NjbPlaylist(const NjbPlaylist& _copy):
    m_playlist(0)
{
    // 	debug() << __PRETTY_FUNCTION__ << " this=" << this << " m_playlist=" << m_playlist << " playlist=" << _copy.m_playlist << endl;
    setPlaylist( _copy.m_playlist );
}

NjbPlaylist::~NjbPlaylist( void)
{
    // 	debug() << __PRETTY_FUNCTION__ << " this=" << this << " m_playlist=" << m_playlist << endl;

    if ( m_playlist )
        NJB_Playlist_Destroy( m_playlist);
}

void
NjbPlaylist::operator=( const NjbPlaylist& _copy)
{
    // 	debug() << __PRETTY_FUNCTION__ << " this=" << this << " m_playlist " << m_playlist << " playlist=" << _copy.m_playlist << endl;

    setPlaylist( _copy.m_playlist );
}

void
NjbPlaylist::setPlaylist( njb_playlist_t* _newlist )
{
    // 	debug() << __PRETTY_FUNCTION__ << " this=" << this << endl;

    //
    // This function copys the new playlist BY VALUE over our current list.
    // It's needed so that objects of this class can be stored in a QValueList
    //

    // First, kill the existing list
    if ( m_playlist )
    {
        NJB_Playlist_Destroy( m_playlist);
    }

    // Get a new one
    m_playlist = NJB_Playlist_New();

    // Set the name
    NJB_Playlist_Set_Name( m_playlist, _newlist->name );

    // Set the id
    m_playlist->plid = _newlist->plid;

    // Iterate through the new list
    NJB_Playlist_Reset_Gettrack( _newlist );
    njb_playlist_track_t* track = NJB_Playlist_Gettrack( _newlist  );
    while ( track )
    {
        // Add a new playlist track to our list
        NJB_Playlist_Addtrack( m_playlist, NJB_Playlist_Track_New( track->trackid ), NJB_PL_END );

        // And move on...
        track = NJB_Playlist_Gettrack( _newlist  );
    }
    debug() << __PRETTY_FUNCTION__ << " OK" << endl;
}

QString
NjbPlaylist::unescapefilename( const QString& _in )
{
    QString result = _in;

    result.replace("%2f","/");

    return result;
}

QString
NjbPlaylist::escapefilename( const QString& _in )
{
    QString result = _in;

    result.replace("/","%2f");

    return result;
}

int
NjbPlaylist::setName( const QString& fileName)
{
    QString playlistName = fileName;
    if( fileName.right( 4) == ".m3u")
        playlistName.truncate( playlistName.length() - 4);

    /*	char** result;
        int nrow, ncolumn;
        char* errmsg;
        sqlite_get_table_printf( m_kionjb->m_db,
        "SELECT id FROM playlists WHERE name='%q'",
        &result, &nrow, &ncolumn, &errmsg,
        playlistName.latin1());
        if( errmsg) {
        m_kionjb->warning( errmsg);
        free( errmsg);
        return ERR_COULD_NOT_READ;
        }

        if( nrow) {
        m_playlist->_state = NJB_PL_CHTRACKS;
        m_playlist->plid = atoi( result[ 1]);
        } else {
        m_playlist->_state = NJB_PL_NEW;
        m_playlist->plid = 0;
        }

        if( playlist_set_name( m_playlist, playlistName) == -1) {
        debug() << "putPlaylist: playlist_set_name failed\n";
        return ERR_COULD_NOT_WRITE;
        }*/

    if ( NJB_Playlist_Set_Name( m_playlist, unescapefilename(fileName).latin1() ) == NJB_FAILURE )
    {
        debug() << __PRETTY_FUNCTION__ << ": NJB_Playlist_Set_Name failed\n";
        return ERR_COULD_NOT_WRITE;
    }

    return NJB_SUCCESS;
}


int
NjbPlaylist::addTrack( const QString& fileName)
{
    debug() << __PRETTY_FUNCTION__ << " filename=" << fileName << endl;
/*
    trackValueList::const_iterator it_track = theTracks->findTrackByName( fileName );
    if( it_track == theTracks->end() ) {
        // couldn't find this track, skip it
        debug() << "putPlaylist: couldn't find " << fileName << endl;
        return NJB_FAILURE;
    }
    njb_playlist_track_t* pl_track = NJB_Playlist_Track_New( (*it_track)->id());
    if( !pl_track) {
        debug() << "putPlaylist: playlist_track_new failed\n";
        return ERR_COULD_NOT_WRITE;
    }
    NJB_Playlist_Addtrack( m_playlist, pl_track, NJB_PL_END);*/
    return NJB_SUCCESS;
}

void
playlist_dump( njb_playlist_t* playlist )
{
    debug() << __PRETTY_FUNCTION__ << endl;

    debug() << "name: " << playlist->name << endl;
    debug() << "state: " << playlist->_state << endl;
    debug() << "ntracks: " << playlist->ntracks << endl;
    debug() << "plid: " << playlist->plid << endl;

    NJB_Playlist_Reset_Gettrack( playlist );
    njb_playlist_track_t* track = NJB_Playlist_Gettrack( playlist );
    while ( track )
    {
        debug() << "track: " << track->trackid << endl;
        track = NJB_Playlist_Gettrack( playlist );
    }
    debug() << __PRETTY_FUNCTION__ << " done" << endl;
}


int
NjbPlaylist::update( void)
{
    // 	debug() << "putPlaylist: state = " << m_playlist->_state << endl;
    // 	debug() << "putPlaylist: id = " << m_playlist->plid << endl;
    debug() << "putPlaylist: sending...\n";

    playlist_dump( m_playlist );
    int status = NJB_Update_Playlist( NjbMediaDevice::theNjb(), m_playlist);
    if( status == -1) {
        debug() << "putPlaylist: NJB_Update_Playlist failed\n";
        if (NJB_Error_Pending(NjbMediaDevice::theNjb()))
        {
            const char* error;
            while ((error = NJB_Error_Geterror(NjbMediaDevice::theNjb())))
                kdError( 7182) << __func__ << ": " << error << endl;
        }
        else
            debug() << __func__ << ": No reason for failure reported.\n";
        return ERR_COULD_NOT_WRITE;
    }
    return NJB_SUCCESS;
}

QStringList
NjbPlaylist::trackNames( void ) const
{
    QStringList result;
/*
    // find tracks in trackList by their ID
    MetaBundle bundle;
    NjbTrack track;
    njb_playlist_track_t *it_pltrack = m_playlist->first;
    while ( it_pltrack) 
    {
        trackValueList::const_iterator it_track = theTracks->findTrackById( it_pltrack->trackid );
        if ( it_track != theTracks->end() )
        {
//            result += it_track.item().bundle()->title();
        }
        it_pltrack = it_pltrack->next;
    }*/

    return result;
}

bool
NjbPlaylist::operator==(const QString& name) const
{
    return escapefilename(m_playlist->name) == name;
}

bool
NjbPlaylist::operator==(const NjbPlaylist& rval) const
{
    return getName() == rval.getName();
}

QString
NjbPlaylist::getName(void) const
{
    debug() << __PRETTY_FUNCTION__ << " this=" << this << " list=" << m_playlist << endl;

    return escapefilename(m_playlist->name);
}

/* ------------------------------------------------------------------------ */
/** Transfer playlists info from the njb to a local structure */
int
playlistValueList::readFromDevice( void)
{


    // ONLY read from the device if this list is empty.

    int playlists = 0;
    NJB_Reset_Get_Playlist( NjbMediaDevice::theNjb());
    while( njb_playlist_t* pl = NJB_Get_Playlist( NjbMediaDevice::theNjb()) ) {
        // FIXME (acejones) Make this a signal
        // 		infoMessage( i18n( "Downloading playlist %1...").arg( ++playlists));
        ++playlists;
        append( NjbPlaylist(pl));
        NJB_Playlist_Destroy( pl);
    }

    debug() << __func__ << ": cached " << playlists << " playlist(s)\n";
    return NJB_SUCCESS;
}

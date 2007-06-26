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

#include "debug.h"
#include "track.h"
#include "njbmediadevice.h"

#include <qregexp.h>
#include <qptrlist.h>

#include <klocale.h>


/* ------------------------------------------------------------------------ */
NjbTrack::NjbTrack( njb_songid_t* song)
{
    njb_songid_frame_t* frame;

    m_id = song->trid;

    MetaBundle *bundle = new MetaBundle();
    frame = NJB_Songid_Findframe( song, FR_SIZE );
    if ( frame->type == NJB_TYPE_UINT32 )
        bundle->setFilesize( frame->data.u_int32_val );
    else
    {
        bundle->setFilesize( 0 );
    error() << " Unexpected frame type:" << frame->type << endl;
    }

    frame = NJB_Songid_Findframe( song, FR_LENGTH );
    if ( frame->type == NJB_TYPE_UINT16 )
        bundle->setLength( frame->data.u_int16_val );
    else
    {
        bundle->setLength( 0 );
    error() << " Unexpected frame type:" << frame->type << endl;
    }

    frame = NJB_Songid_Findframe( song, FR_GENRE );
    if( frame )
    {
        bundle->setGenre( AtomicString( frame->data.strval ) );
    }

    frame = NJB_Songid_Findframe( song, FR_ARTIST );
    if( frame )
    {
        QString artist = QString::fromUtf8( frame->data.strval );
        artist.replace( QRegExp( "/" ), "-" );
        bundle->setArtist( artist );
    }
    else
        bundle->setArtist( i18n("Unknown artist") );

    frame = NJB_Songid_Findframe( song, FR_ALBUM );
    if( frame)
    {
        QString album = QString::fromUtf8( frame->data.strval );
        album.replace( QRegExp( "/" ), "-" );
        bundle->setAlbum( album );
    }
    else
        bundle->setAlbum( i18n("Unknown album") );

    frame = NJB_Songid_Findframe( song, FR_TITLE);
    if( frame )
    {
        QString title = QString::fromUtf8( frame->data.strval );
        title.replace( QRegExp( "/"), "-");
        bundle->setTitle( title );
    }
    else
        bundle->setTitle( i18n("Unknown title") );

    frame = NJB_Songid_Findframe( song, FR_TRACK );
    if( frame )
    {
        switch ( frame->type )
        {
        case NJB_TYPE_UINT16:
            bundle->setTrack( frame->data.u_int16_val );
            break;
        case NJB_TYPE_UINT32:
            bundle->setTrack( frame->data.u_int32_val );
            break;
        case NJB_TYPE_STRING:
            bundle->setTrack( QString::fromUtf8(frame->data.strval).toUInt() );
            break;
        default:
            bundle->setTrack( 0 );
        }
    }

    QString codec;
    frame = NJB_Songid_Findframe( song, FR_CODEC);
    if( frame )
    {
        codec = QCString( frame->data.strval).lower();
        if( codec == "mp3" )
            bundle->setFileType( MetaBundle::mp3 );
        else if (codec == "wma" )
            bundle->setFileType( MetaBundle::wma );
        else
            bundle->setFileType( MetaBundle::other ); // It's a wav...
    }
    else
    {
        bundle->setFileType( MetaBundle::mp3 ); //Assumed...
        codec = "mp3"; // Used for the next frame as a fallback
    }

    frame = NJB_Songid_Findframe( song, FR_FNAME );
    QString filename;
    if( frame )
    {
        //bundle->setUrl( KURL( frame->data.strval ) );
        filename = QString::fromUtf8( frame->data.strval );

    }
    if( filename.isEmpty() )
    {
        //bundle->setUrl( bundle->artist() + " - " + bundle->title() + '.' + codec );
        filename = bundle->artist() + " - " + bundle->title() + '.' + codec;

    }
    bundle->setPath( filename );

    frame = NJB_Songid_Findframe( song, FR_YEAR );
    if( frame )
    {
        switch ( frame->type )
        {
        case NJB_TYPE_UINT16:
            bundle->setYear( frame->data.u_int16_val );
            break;
        case NJB_TYPE_UINT32:
            bundle->setYear( frame->data.u_int32_val );
            break;
        case NJB_TYPE_STRING:
            bundle->setYear( QString::fromUtf8( frame->data.strval ).toInt() );
            break;
        default:
            bundle->setYear( 0 );
        }
    }
    this->setBundle( *bundle );
}

NjbTrack::~NjbTrack()
{
    ItemList.setAutoDelete( true );
    while( ItemList.count() > 0 )
    {
        delete ItemList.first();
    }
}

void
NjbTrack::writeToSongid( njb_songid_t *songid )
{
    NJB_Songid_Addframe( songid, NJB_Songid_Frame_New_Filename( m_bundle.filename().utf8() ) );
    NJB_Songid_Addframe( songid, NJB_Songid_Frame_New_Filesize( m_bundle.filesize() ) );
    NJB_Songid_Addframe( songid, NJB_Songid_Frame_New_Codec( "mp3" ) ); //for now
    NJB_Songid_Addframe( songid, NJB_Songid_Frame_New_Title( m_bundle.title().utf8() ) );
    NJB_Songid_Addframe( songid, NJB_Songid_Frame_New_Album( m_bundle.album().ptr()->utf8() ) );
    NJB_Songid_Addframe( songid, NJB_Songid_Frame_New_Genre( m_bundle.genre().ptr()->utf8() ) );
    NJB_Songid_Addframe( songid, NJB_Songid_Frame_New_Artist( m_bundle.artist().ptr()->utf8() ) );
    NJB_Songid_Addframe( songid, NJB_Songid_Frame_New_Length( m_bundle.length() ) );
    NJB_Songid_Addframe( songid, NJB_Songid_Frame_New_Tracknum( m_bundle.track() ) );
    NJB_Songid_Addframe( songid, NJB_Songid_Frame_New_Year( m_bundle.year() ) );
}

njb_songid_t *
NjbTrack::newSongid()
{
    njb_songid_t *songid = new njb_songid_t;
    writeToSongid( songid );
    return songid;
}

void
NjbTrack::setBundle( MetaBundle &bundle )
{
    if( bundle.title().isEmpty() )
        bundle.setTitle( i18n( "Unknown title" ) );
    if( bundle.artist().isEmpty() )
        bundle.setArtist( i18n( "Unknown artist" ) );
    if( bundle.album().isEmpty() )
        bundle.setAlbum( i18n( "Unknown album" ) );
    if( bundle.genre().isEmpty() )
        bundle.setGenre( i18n( "Unknown genre" ) );

    m_bundle = bundle;
}

void
NjbTrack::addItem( const NjbMediaItem *item )
{
    ItemList.append( item );
}

bool
NjbTrack::removeItem( const NjbMediaItem *item )
{
    ItemList.remove( item );
    debug() << "Removed item\n";
    return ItemList.isEmpty();
}

/* ------------------------------------------------------------------------ */
trackValueList::iterator
trackValueList::findTrackByName( const QString& _filename )
{
    trackValueList::iterator it;
    for( it = begin(); it != end(); it++)
        if( (*it)->bundle()->url().path() == _filename)
            break;
    return it;
}

/* ------------------------------------------------------------------------ */
trackValueList::iterator
trackValueList::findTrackById( unsigned _id )
{
    trackValueList::iterator it;
    for( it = begin(); it != end(); it++)
        if( (*it)->id() == _id)
            break;
    return it;
}

/* ------------------------------------------------------------------------ */
trackValueList::const_iterator
trackValueList::findTrackById( unsigned _id ) const
{
    trackValueList::const_iterator it;
    for( it = begin(); it != end(); it++)
        if( (*it)->id() == _id)
            break;
    return it;
}

/* ------------------------------------------------------------------------ */
/** Transfer musical info from the njb to local structures */
int
trackValueList::readFromDevice( void )
{
    int i = 0;

    // Don't get extended metadatas

    NJB_Get_Extended_Tags(NjbMediaDevice::theNjb(), 0);
    NJB_Reset_Get_Track_Tag( NjbMediaDevice::theNjb());
    while( njb_songid_t* song = NJB_Get_Track_Tag( NjbMediaDevice::theNjb()))
    {
        NjbTrack *track = new NjbTrack( song );
        append( track );
        NJB_Songid_Destroy( song );

        ++i;
    }
    debug() << ": " << i << " jukebox tracks loaded from device." << endl;

    return NJB_SUCCESS;
}

//Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
//Copyright:  See COPYING file that comes with this distribution
//

#include <qstring.h>
#include "metabundle.h"
#include "playlistitem.h"
#include <taglib/tag.h>
#include <taglib/tstring.h>
#include <taglib/audioproperties.h>

/*
 * This class is not very complete, it fits our needs as they stand currently
 * If it doesn't work for you in some way, extend it sensibly :)
 */


//TODO one without audioProps please
//TODO have ability to determine bitrate etc from the strings, slow but infrequently called so ok
//TODO cache prettyLength for bundles
//TODO is it feasible to just use a TagLib::AudioProperties structure?


//TitleProxy ctor
MetaBundle::MetaBundle( const QString& title,
                        const QString& url,
                        const int      bitrate,
                        const QString& genre,
                        const QString& streamName,
                        const QString& streamUrl )
  : m_url       ( streamUrl )
  , m_title     ( url + title )
  , m_genre     ( genre )
  , m_bitrate   ( bitrate )
  , m_length    ( Undetermined )
  , m_sampleRate( Undetermined )
{}

//PlaylistItem ctor
MetaBundle::MetaBundle( const PlaylistItem *item, TagLib::AudioProperties *ap )
  : m_url(     item->url() )
  , m_title(   item->title() )        //because you override text()
  , m_artist(  item->exactText( 2 ) ) //because you override text()
  , m_album(   item->exactText( 3 ) ) //etc.
  , m_year(    item->exactText( 4 ) ) //..
  , m_comment( item->exactText( 5 ) ) //.
  , m_genre(   item->exactText( 6 ) )
  , m_track(   item->exactText( 7 ) )
{
    init( ap );
}

//Taglib::Tag ctor
MetaBundle::MetaBundle( const KURL &url, TagLib::Tag *tag, TagLib::AudioProperties *ap )
  : m_url( url )
  , m_title(  TStringToQString( tag->title() ).stripWhiteSpace() )
  , m_artist( TStringToQString( tag->artist() ).stripWhiteSpace() )
  , m_album(  TStringToQString( tag->album() ).stripWhiteSpace() )
  , m_year(   tag->year() ? QString::number( tag->year() ) : QString::null )
  , m_comment( TStringToQString( tag->comment() ).stripWhiteSpace() )
  , m_genre(   TStringToQString( tag->genre() ).stripWhiteSpace() )
  , m_track(   tag->track() ? QString::number( tag->track() ) : QString::null )
{
    init( ap );
}

void
MetaBundle::init( TagLib::AudioProperties *ap )
{
    if( ap )
    {
        m_bitrate    = ap->bitrate();
        m_length     = ap->length();
        m_sampleRate = ap->sampleRate();
    }
    else m_bitrate = m_length = m_sampleRate = Undetermined;
}

QString
MetaBundle::prettyTitle() const
{
    QString s = m_artist;
    if( !s.isEmpty() ) s += " - ";
    s += m_title;

    if( s.isEmpty() )
    {
        //remove file extension and tidy
        s = m_url.fileName().left( s.findRev( '.' ) ).replace( '_', ' ' );
    }

    return s;
}

QString
MetaBundle::prettyLength( int length ) //static
{
    //NOTE don't inline this function!

    QString s;

    if( length > 0 )
    {
        //we don't do hours, people aren't interested in them
        int min = length / 60;
        int sec = length % 60;

        //don't zeroPad the minutes
        s.setNum( min ).append( ':' );
        if( sec < 10 ) s += '0';
        s += QString::number( sec );
    }
    else if( length == Unavailable ) s = '?';
    else if( length == Irrelevant  ) s = '-';

    return s;
}

QString
MetaBundle::prettyGeneric( const QString &s, int i ) //static
{
    //TODO ensure this inlines

    return ( i > 0 ) ? s.arg( i ) : ( i == Undetermined ) ? QString() : "?";
}

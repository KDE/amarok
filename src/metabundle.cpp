//Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
//Copyright:  See COPYING file that comes with this distribution
//

#include "metabundle.h"
#include "playlistitem.h"

#include <qstring.h>

#include <kfilemetainfo.h>

#include <taglib/tag.h>
#include <taglib/fileref.h>
#include <taglib/tstring.h>
#include <taglib/audioproperties.h>

/*
 * This class is not very complete, it fits our needs as they stand currently
 * If it doesn't work for you in some way, extend it sensibly :)
 */


 //FIXME these aren't i18n'd
 //the point here is to force sharing of these strings returned from prettyBitrate()
static const QString bitrateStore[9] = { "?", "32 kbps", "64 kbps", "96 kbps", "128 kbps", "160 kbps", "192 kbps", "224 kbps", "256 kbps" };

//TODO consider the worth of this extension; use trackStore.ref( i )
//static const QString trackStore = "0123456789";


//TODO one without audioProps please
//TODO have ability to determine bitrate etc from the strings, slow but infrequently called so ok
//TODO cache prettyLength for bundles
//TODO is it feasible to just use a TagLib::AudioProperties structure?


//TitleProxy ctor
MetaBundle::MetaBundle( const QString& title,
                        const QString& url,
                        const int      bitrate,
                        const QString& genre,
                        const QString& /*streamName*/,
                        const QString& streamUrl )
  : m_url       ( streamUrl )
  , m_title     ( /*url + QString( " -- " ) + */title )
  , m_genre     ( genre )
  , m_bitrate   ( bitrate )
  , m_length    ( Irrelevant )
  , m_sampleRate( Unavailable )
{}

//PlaylistItem ctor
MetaBundle::MetaBundle( const PlaylistItem *item )
  : m_url    ( item->url() )
  , m_title  ( item->title() )        //because you override text()
  , m_artist ( item->exactText( 2 ) ) //because you override text()
  , m_album  ( item->exactText( 3 ) ) //etc.
  , m_year   ( item->exactText( 4 ) ) //..
  , m_comment( item->exactText( 5 ) ) //.
  , m_genre  ( item->exactText( 6 ) )
  , m_track  ( item->exactText( 7 ) )
{
    if( m_url.isLocalFile() )
    {
        TagLib::FileRef f( m_url.path().local8Bit(), true, TagLib::AudioProperties::Accurate );

        if ( f.isNull() )
        {
            KFileMetaInfo info( m_url, QString::null, KFileMetaInfo::Everything );

            if ( info.isValid() )
            {
                m_bitrate    = info.item( "Bitrate" ).value().toInt();
                m_length     = info.item( "Length" ).value().toInt();
                m_sampleRate = info.item( "Sample Rate" ).value().toInt();
            }
        }
        else init( f.audioProperties() );

    } else { //is Stream

        //FIXME not correct handling, say is ftp://file
        m_bitrate    = item->exactText( 10 ).left( 3 ).toInt();
        m_sampleRate = Undetermined;
        m_length     = Unavailable;
    }
}

//Taglib::Tag ctor //TODO DEPRECATE
MetaBundle::MetaBundle( const KURL &url, TagLib::Tag *tag, TagLib::AudioProperties *ap )
  : m_url( url )
  , m_title(   TStringToQString( tag->title() ).stripWhiteSpace() )
  , m_artist(  TStringToQString( tag->artist() ).stripWhiteSpace() )
  , m_album(   TStringToQString( tag->album() ).stripWhiteSpace() )
  , m_year(    tag->year() ? QString::number( tag->year() ) : QString::null )
  , m_comment( TStringToQString( tag->comment() ).stripWhiteSpace() )
  , m_genre(   TStringToQString( tag->genre() ).stripWhiteSpace() )
  , m_track(   tag->track() ? QString::number( tag->track() ) : QString::null )
{
    init( ap );
}

void
MetaBundle::init( TagLib::AudioProperties *ap )
{
    if( !m_url.isLocalFile() ) return;

    if( ap )
    {
        m_bitrate    = ap->bitrate();
        m_length     = ap->length();
        m_sampleRate = ap->sampleRate();
    }
    else m_bitrate = m_length = m_sampleRate = Undetermined;
}

void
MetaBundle::init( const KFileMetaInfo& info )
{
    if( info.isValid() )
    {
        m_title      = info.item( "Title" ).string();
        m_artist     = info.item( "Artist" ).string();
        m_album      = info.item( "Album" ).string();
        m_year       = info.item( "Year" ).string();
        m_comment    = info.item( "Comment" ).string();
        m_genre      = info.item( "Genre" ).string();
        m_track      = info.item( "Track" ).string();
        m_bitrate    = info.item( "Bitrate" ).value().toInt();
        m_length     = info.item( "Length" ).value().toInt();
        m_sampleRate = info.item( "Sample Rate" ).value().toInt();
    }
    else m_bitrate = m_length = m_sampleRate = Undetermined;
}

MetaBundle&
MetaBundle::readTags( bool audioProperties )
{
    //TODO detect mimetype and use specfic reader like Scott recommends

   TagLib::FileRef f( m_url.path().local8Bit(), audioProperties, TagLib::AudioProperties::Fast );

   if ( !f.isNull() )
   {
        if( f.tag() )
        {
            TagLib::Tag *tag = f.tag();

            m_title   = TStringToQString( tag->title() ).stripWhiteSpace();
            m_artist  = TStringToQString( tag->artist() ).stripWhiteSpace();
            m_album   = TStringToQString( tag->album() ).stripWhiteSpace();
            m_comment = TStringToQString( tag->comment() ).stripWhiteSpace();
            m_genre   = TStringToQString( tag->genre() ).stripWhiteSpace();
            m_year    = tag->year() ? QString::number( tag->year() ) : QString::null;
            m_track   = tag->track() ? QString::number( tag->track() ) : QString::null;
        }
        init( f.audioProperties() );
    }
    else init( KFileMetaInfo( m_url, QString::null, KFileMetaInfo::Everything ) );

    return *this;
}

QString
MetaBundle::prettyTitle() const
{
    //NOTE this gets regressed often, please be careful!
    //NOTE whatever you do, handle the stream case, streams have no artist but have an excellent title

    QString s = m_artist;
    if( !s.isEmpty() ) s += " - ";
    s += m_title;
    if( s.isEmpty() ) s = prettyTitle( m_url.fileName() );
    return s;
}

QString
MetaBundle::prettyTitle( QString filename ) //static
{
    QString &s = filename; //just so the code is more readable

    //remove file extension, s/_/ /g and decode %2f-like sequences
    s = s.left( s.findRev( '.' ) ).replace( '_', ' ' );
    s = KURL::decode_string(s);

    return s;
}

QString
MetaBundle::prettyLength( int seconds ) //static
{
    QString s;

    if( seconds > 0 ) s = prettyTime( seconds, false );
    else if( seconds == Unavailable ) s = '?';
    else if( seconds == Irrelevant  ) s = '-';

    return s; //Undetermined = ""
}

QString
MetaBundle::prettyTime( int seconds, bool showHours ) //static
{
    QString s = QChar( ':' );
    s.append( zeroPad( seconds % 60 ) ); //seconds
    seconds /= 60;

    if( showHours )
    {
        s.prepend( zeroPad( seconds % 60 ) ); //minutes
        s.prepend( ':' );
        seconds /= 60;
    }

    //don't zeroPad the last one, as it can be greater than 2 digits
    s.prepend( QString::number( seconds ) ); //hours or minutes depending on above if block

    return s;
}

QString
MetaBundle::prettyBitrate( int i )
{
    return ( i % 32 == 0 && i < 257 ) ? bitrateStore[ i /32 ] : prettyGeneric( i18n( "Bitrate", "%1 kbps" ), i );
}

QString
MetaBundle::prettyGeneric( const QString &s, int i ) //static
{
    //TODO ensure this inlines

    return ( i > 0 ) ? s.arg( i ) : ( i == Undetermined ) ? "?" : QString::null;
}

//Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
//Copyright:  See COPYING file that comes with this distribution
//

#define DEBUG_PREFIX "MetaBundle"

#include "amarokconfig.h"
#include "collectiondb.h"
#include <kfilemetainfo.h>
#include <kmimetype.h>
#include "metabundle.h"
#include "playlistitem.h"
#include <qfile.h> //decodePath()
#include <taglib/fileref.h>
#include <taglib/id3v1genres.h> //used to load genre list
#include <taglib/mpegfile.h>
#include <taglib/tag.h>
#include <taglib/tstring.h>


MetaBundle::MetaBundle( const KURL &url, TagLib::AudioProperties::ReadStyle readStyle )
    : m_url( url )
    , m_exists( url.protocol() == "file" && QFile::exists( url.path() ) )
    , m_isValidMedia( false ) //will be updated
{
    if ( m_exists ) {
        m_isValidMedia = CollectionDB::instance()->bundleForUrl( this );

        if ( !m_isValidMedia || length() <= 0 )
            readTags( readStyle );
    }
    else
        m_bitrate = m_length = m_sampleRate = Unavailable;
}

//StreamProvider ctor
MetaBundle::MetaBundle( const QString& title,
                        const QString& streamUrl,
                        const int      bitrate,
                        const QString& genre,
                        const QString& streamName,
                        const KURL& url )
        : m_url       ( url )
        , m_genre     ( genre )
        , m_streamName( streamName )
        , m_streamUrl ( streamUrl )
        , m_bitrate   ( bitrate )
        , m_length    ( Irrelevant )
        , m_sampleRate( Unavailable )
        , m_exists( true )
        , m_isValidMedia( true )
{
   if( title.contains( '-' ) ) {
       m_title  = title.section( '-', 1, 1 ).stripWhiteSpace();
       m_artist = title.section( '-', 0, 0 ).stripWhiteSpace();
   }
   else {
       m_title  = title;
       m_artist = streamName; //which is sort of correct..
   }
}

///PlaylistItem ctor
/// NOT THREAD-SAFE!!
MetaBundle::MetaBundle( const PlaylistItem *item )
        : m_url    ( item->url() )
        , m_title  ( item->title() )        //because you override text()
        , m_artist ( item->exactText( 2 ) ) //because you override text()
        , m_album  ( item->exactText( 3 ) ) //etc.
        , m_year   ( item->exactText( 4 ) ) //..
        , m_comment( item->exactText( 5 ) ) //.
        , m_genre  ( item->exactText( 6 ) )
        , m_track  ( item->exactText( 7 ) )
        , m_exists ( true ) //FIXME
        , m_isValidMedia( true )
{
    if( m_url.protocol() == "file" )
        readTags( TagLib::AudioProperties::Accurate );

    else {
        // is a stream
        //FIXME not correct handling, say is ftp://file
        m_bitrate    = item->exactText( 10 ).left( 3 ).toInt();
        m_sampleRate = Undetermined;
        m_length     = Irrelevant;
    }
}

bool
MetaBundle::operator==( const MetaBundle& bundle )
{
    return m_artist     == bundle.artist() &&
           m_title      == bundle.title() &&
           m_album      == bundle.album() &&
           m_year       == bundle.year() &&
           m_comment    == bundle.comment() &&
           m_genre      == bundle.genre() &&
           m_track      == bundle.track() &&
           m_bitrate    == bundle.bitrate() &&
           m_sampleRate == bundle.sampleRate();
}

void
MetaBundle::init( TagLib::AudioProperties *ap )
{
    if ( ap ) {
        m_bitrate    = ap->bitrate();
        m_length     = ap->length();
        m_sampleRate = ap->sampleRate();
    }
    else
        m_bitrate = m_length = m_sampleRate = Undetermined;
}

void
MetaBundle::init( const KFileMetaInfo& info )
{
    if( info.isValid() && !info.isEmpty() )
    {
        m_artist     = info.item( "Artist" ).string();
        m_album      = info.item( "Album" ).string();
        m_year       = info.item( "Year" ).string();
        m_comment    = info.item( "Comment" ).string();
        m_genre      = info.item( "Genre" ).string();
        m_track      = info.item( "Track" ).string();
        m_bitrate    = info.item( "Bitrate" ).value().toInt();
        m_length     = info.item( "Length" ).value().toInt();
        m_sampleRate = info.item( "Sample Rate" ).value().toInt();

        // For title, check if it is valid. If not, use prettyTitle.
        // @see bug:83650
        const KFileMetaInfoItem item = info.item( "Title" );
        m_title = item.isValid() ? item.string() : prettyTitle( m_url.fileName() );

        // because whoever designed KMetaInfoItem is a donkey
        #define makeSane( x ) if( x == "---" ) x = null;
        QString null;
        makeSane( m_artist );
        makeSane( m_album );
        makeSane( m_year );
        makeSane( m_comment );
        makeSane( m_genre  );
        makeSane( m_track );
        makeSane( m_title );
        #undef makeSane

        m_isValidMedia = true;
    }
    else {
        m_bitrate = m_length = m_sampleRate = Undetermined;
        m_isValidMedia = false;
    }
}

void
MetaBundle::readTags( TagLib::AudioProperties::ReadStyle readStyle )
{
    if( m_url.protocol() != "file" )
        return;

    const QString path = m_url.path();
    TagLib::FileRef fileref;
    TagLib::Tag *tag = 0;

    if( AmarokConfig::recodeID3v1Tags() && path.endsWith( ".mp3", false ) )
    {
        TagLib::MPEG::File *mpeg = new TagLib::MPEG::File( QFile::encodeName( path ), true, readStyle );
        fileref = TagLib::FileRef( mpeg );

        if( mpeg->isValid() )
            // we prefer ID3v1 over ID3v2 if recoding tags because
            // apparently this is what people who ignore ID3 standards want
            tag = mpeg->ID3v1Tag() ? (TagLib::Tag*)mpeg->ID3v1Tag() : (TagLib::Tag*)mpeg->ID3v2Tag();
    }

    else {
        fileref = TagLib::FileRef( QFile::encodeName( path ), true, readStyle );

        if( !fileref.isNull() )
            tag = fileref.tag();
    }

    if( !fileref.isNull() ) {
        if ( tag ) {
            #define strip( x ) TStringToQString( x ).stripWhiteSpace()
            m_title   = strip( tag->title() );
            m_artist  = strip( tag->artist() );
            m_album   = strip( tag->album() );
            m_comment = strip( tag->comment() );
            m_genre   = strip( tag->genre() );
            m_year    = tag->year() ? QString::number( tag->year() ) : QString();
            m_track   = tag->track() ? QString::number( tag->track() ) : QString();
            #undef strip

            m_isValidMedia = true;
        }

        init( fileref.audioProperties() );
    }

    //FIXME disabled for beta4 as it's simpler to not got 100 bug reports
    //else if( KMimeType::findByUrl( m_url )->is( "audio" ) )
    //    init( KFileMetaInfo( m_url, QString::null, KFileMetaInfo::Everything ) );
}

QString
MetaBundle::prettyTitle() const
{
    QString s = m_artist;

    //NOTE this gets regressed often, please be careful!
    //     whatever you do, handle the stream case, streams have no artist but have an excellent title

    //FIXME doesn't work for resume playback

    if( !s.isEmpty() ) s += i18n(" - ");
    s += m_title;
    if( s.isEmpty() ) s = prettyTitle( m_url.fileName() );
    return s;
}

QString
MetaBundle::veryNiceTitle() const
{
    QString s;
    //NOTE I'm not sure, but the notes and FIXME's in the prettyTitle function should be fixed now.
    //     If not then they do apply to this function also!
    if( !m_title.isEmpty() )
    {
        if( !m_artist.isEmpty() )
            s = i18n( "%1 by %2" ).arg( m_title ).arg( m_artist );
        else
            s = m_title;
    }
    else
    {
        s = prettyTitle( m_url.fileName() );
    }
    return s;
}

QString
MetaBundle::infoByColumn( int column, bool pretty ) const
{
    switch( column )
    {
        case PlaylistItem::Filename:  return filename();
        case PlaylistItem::Title:     return title();
        case PlaylistItem::Artist:    return artist();
        case PlaylistItem::Album:     return album();
        case PlaylistItem::Year:      return year();
        case PlaylistItem::Comment:   return comment();
        case PlaylistItem::Genre:     return genre();
        case PlaylistItem::Track:     return track();
        case PlaylistItem::Directory: return directory();
        case PlaylistItem::Length:    return pretty ? prettyLength() : QString::number( length() );
        case PlaylistItem::Bitrate:   return pretty ? prettyBitrate() : QString::number( bitrate() );
        case PlaylistItem::Type:      return type( pretty );
    }
    return QString::null;
}

QString
MetaBundle::prettyTitle( const QString &filename ) //static
{
    QString s = filename; //just so the code is more readable

    //remove file extension, s/_/ /g and decode %2f-like sequences
    s = s.left( s.findRev( '.' ) ).replace( '_', ' ' );
    s = KURL::decode_string( s );

    return s;
}

QString
MetaBundle::prettyLength( int seconds, bool showHours ) //static
{
    if( seconds > 0 ) return prettyTime( seconds, showHours );
    if( seconds == Undetermined ) return "?";
    if( seconds == Irrelevant  ) return "-";

    return QString::null; //Unavailable = ""
}

QString
MetaBundle::prettyTime( uint seconds, bool showHours ) //static
{
    QString s = QChar( ':' );
    s.append( zeroPad( seconds % 60 ) ); //seconds
    seconds /= 60;

    if( showHours && seconds >= 60)
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
    //the point here is to force sharing of these strings returned from prettyBitrate()
    static const QString bitrateStore[9] = {
            "?", "32 kbps", "64 kbps", "96 kbps", "128 kbps",
            "160 kbps", "192 kbps", "224 kbps", "256 kbps" };

    return (i >=0 && i <= 256 && i % 32 == 0)
                ? bitrateStore[ i / 32 ]
                : prettyGeneric( i18n( "Bitrate", "%1 kbps" ), i );
}

QStringList
MetaBundle::genreList() //static
{
    QStringList list;

    TagLib::StringList genres = TagLib::ID3v1::genreList();
    for( TagLib::StringList::ConstIterator it = genres.begin(), end = genres.end(); it != end; ++it )
        list += TStringToQString( (*it) );

    list.sort();

    return list;
}

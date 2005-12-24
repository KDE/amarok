// Max Howell <max.howell@methylblue.com>, (C) 2004
// Alexandre Pereira de Oliveira <aleprj@gmail.com>, (C) 2005
// License: GNU General Public License V2


#define DEBUG_PREFIX "MetaBundle"

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
#include <taglib/id3v2tag.h>
#include <taglib/id3v1tag.h>
#include <taglib/xiphcomment.h>
#include <taglib/mpegfile.h>
#include <taglib/oggfile.h>
#include <taglib/vorbisfile.h>
#include <taglib/textidentificationframe.h>
#include <taglib/xiphcomment.h>
#include <taglib/tbytevector.h>


MetaBundle::MetaBundle( const KURL &url, bool noCache, TagLib::AudioProperties::ReadStyle readStyle )
    : m_url( url )
    , m_year( 0 )
    , m_track( 0 )
    , m_discNumber( 0 )
    , m_exists( url.protocol() == "file" && QFile::exists( url.path() ) )
    , m_isValidMedia( false ) //will be updated
{
    if ( m_exists ) {
        if ( !noCache )
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
        , m_year( 0 )
        , m_track( 0 )
        , m_bitrate   ( bitrate )
        , m_length    ( Irrelevant )
        , m_sampleRate( Unavailable )
        , m_discNumber( 0 )
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
        , m_title  ( item->title() )
        , m_artist ( item->artist() )
        , m_album  ( item->album() )
        , m_comment( item->comment() )
        , m_genre  ( item->genre() )
        , m_year   ( item->year() )
        , m_track  ( item->track() )
        , m_discNumber( 0 )
        , m_exists ( true ) //FIXME
        , m_isValidMedia( true )
{
    if( m_url.protocol() == "file" )
        readTags( TagLib::AudioProperties::Accurate );

    else {
        // is a stream
        //FIXME not correct handling, say is ftp://file
        m_bitrate    = item->bitrate();
        m_sampleRate = Undetermined;
        m_length     = Irrelevant;
    }
}

bool
MetaBundle::operator==( const MetaBundle& bundle )
{
    return m_artist     == bundle.artist() &&
           m_title      == bundle.title() &&
           m_composer   == bundle.composer() &&
           m_album      == bundle.album() &&
           m_year       == bundle.year() &&
           m_comment    == bundle.comment() &&
           m_genre      == bundle.genre() &&
           m_track      == bundle.track() &&
           m_discNumber == bundle.discNumber() &&
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
        m_comment    = info.item( "Comment" ).string();
        m_genre      = info.item( "Genre" ).string();
        m_year       = info.item( "Year" ).string().toInt();
        m_track      = info.item( "Track" ).string().toInt();
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
        makeSane( m_comment );
        makeSane( m_genre  );
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


    fileref = TagLib::FileRef( QFile::encodeName( path ), true, readStyle );

    if( !fileref.isNull() ) {
        tag = fileref.tag();

        if ( tag ) {
            #define strip( x ) TStringToQString( x ).stripWhiteSpace()
            m_title   = strip( tag->title() );
            m_artist  = strip( tag->artist() );
            m_album   = strip( tag->album() );
            m_comment = strip( tag->comment() );
            m_genre   = strip( tag->genre() );
            m_year    = tag->year();
            m_track   = tag->track();
            #undef strip

            m_isValidMedia = true;
        }

    /* As mpeg implementation on TagLib uses a Tag class that's not defined on the headers,
       we have to cast the files, not the tags! */

//        if ( dynamic_cast<TagLib::WMA::Tag *>( tag ) )
//            m_type = wma;
        QString disc;
        if ( TagLib::MPEG::File *file = dynamic_cast<TagLib::MPEG::File *>( fileref.file() ) ) {
            m_type = mp3;
            if ( file->ID3v2Tag() ) {
                if ( !file->ID3v2Tag()->frameListMap()["TPOS"].isEmpty() ) {
                    disc = TStringToQString( file->ID3v2Tag()->frameListMap()["TPOS"].front()->toString() ).stripWhiteSpace();
                }
                if ( !file->ID3v2Tag()->frameListMap()["TCOM"].isEmpty() ) {
                    m_composer = TStringToQString( file->ID3v2Tag()->frameListMap()["TCOM"].front()->toString() ).stripWhiteSpace();
                }
            }
        }
        else if ( TagLib::Ogg::Vorbis::File *file = dynamic_cast<TagLib::Ogg::Vorbis::File *>( fileref.file() ) ) {
            m_type = ogg;
            if ( file->tag() ) {
                if ( !file->tag()->fieldListMap()[ "COMPOSER" ].isEmpty() ) {
                    m_composer = TStringToQString( file->tag()->fieldListMap()["COMPOSER"].front() ).stripWhiteSpace();
                }
                if ( !file->tag()->fieldListMap()[ "DISCNUMBER" ].isEmpty() ) {
                    disc = TStringToQString( file->tag()->fieldListMap()["DISCNUMBER"].front() ).stripWhiteSpace();
                }
            }
        }
        else
            m_type = other;

        if ( !disc.isEmpty() ) {
            int i = disc.find ('/');
            if ( i != -1 ) {
                m_discNumber = disc.left( i ).toInt();
                // disc.right( i ).toInt() is total number of discs, we don't use this at the moment
            }
            else {
                m_discNumber = disc.toInt();
            }
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
        case PlaylistItem::Composer:  return composer();
        case PlaylistItem::DiscNumber:return ( pretty && !discNumber() ) ? QString() : QString::number( discNumber() );
        case PlaylistItem::Album:     return album();
        case PlaylistItem::Year:      return ( pretty && !year() ) ? QString() : QString::number( year() );
        case PlaylistItem::Comment:   return comment();
        case PlaylistItem::Genre:     return genre();
        case PlaylistItem::Track:     return ( pretty && !track() ) ? QString() : QString::number( track() );
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
            "?", "32", "64", "96", "128", "160", "192", "224", "256" };

    return (i >=0 && i <= 256 && i % 32 == 0)
                ? bitrateStore[ i / 32 ]
                : prettyGeneric( "%1", i );
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

void
MetaBundle::setExtendedTag( TagLib::File *file, int tag, const QString value ) {
    char *id;
    TagLib::MPEG::File *mpegFile;
    TagLib::Ogg::Vorbis::File *oggFile;

    switch ( m_type ) {
        case mp3:
            switch( tag ) {
                case ( composerTag ): id = "TCOM"; break;
                case ( discNumberTag ): id = "TPOS"; break;
            }
            if ( mpegFile = dynamic_cast<TagLib::MPEG::File *>( file ) ) {
                if ( mpegFile->ID3v2Tag() ) {
                    if ( value.isEmpty() )
                        mpegFile->ID3v2Tag()->removeFrames( id );
                    else {
                        if( !mpegFile->ID3v2Tag()->frameListMap()[id].isEmpty() )
                            mpegFile->ID3v2Tag()->frameListMap()[id].front()->setText( QStringToTString( value ) );
                        else {
                            TagLib::ID3v2::TextIdentificationFrame *frame = new TagLib::ID3v2::TextIdentificationFrame( id, TagLib::ID3v2::FrameFactory::instance()->defaultTextEncoding() );
                            frame->setText( QStringToTString( value ) );
                            mpegFile->ID3v2Tag()->addFrame( frame );
                        }
                    }
                }
            }
            break;
        case ogg:
            switch( tag ) {
                case ( composerTag ): id = "COMPOSER"; break;
                case ( discNumberTag ): id = "DISCNUMBER"; break;
            }
           if ( oggFile = dynamic_cast<TagLib::Ogg::Vorbis::File *>( file ) ) {
                if ( oggFile->tag() ) {
                    if ( value.isEmpty() )
                        oggFile->tag()->removeField( id );
                    else
                        oggFile->tag()->addField( id, QStringToTString( value ), true );
                }
            }
            break;
    }
}

bool
MetaBundle::save() {
    //Set default codec to UTF-8 (see bugs 111246 and 111232)
    TagLib::ID3v2::FrameFactory::instance()->setDefaultTextEncoding(TagLib::String::UTF8);

    QCString path = QFile::encodeName( m_url.path() );
    TagLib::FileRef f( path, false );

    if ( !f.isNull() )
    {
        TagLib::Tag * t = f.tag();
        t->setTitle( QStringToTString( m_title ) );
        t->setArtist( QStringToTString( m_artist ) );
        t->setAlbum( QStringToTString( m_album ) );
        t->setTrack( m_track );
        t->setYear( m_year );
        t->setComment( QStringToTString( m_comment ) );
        t->setGenre( QStringToTString( m_genre ) );

        if ( hasExtendedMetaInformation() ) {
            setExtendedTag( f.file(), composerTag, m_composer );
            setExtendedTag( f.file(), discNumberTag, m_discNumber ? QString::number( m_discNumber ) : QString() );
        }
        return f.save();
    }
    return false;
}

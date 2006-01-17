// Max Howell <max.howell@methylblue.com>, (C) 2004
// Alexandre Pereira de Oliveira <aleprj@gmail.com>, (C) 2005
// License: GNU General Public License V2


#define DEBUG_PREFIX "MetaBundle"

#include "amarok.h"
#include "debug.h"
#include "collectiondb.h"
#include <kfilemetainfo.h>
#include <kmimetype.h>
#include <qdom.h>
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

#include <config.h>
#ifdef HAVE_MP4V2
#include "metadata/mp4/mp4file.h"
#include "metadata/mp4/mp4tag.h"
#else
#include "metadata/m4a/mp4file.h"
#include "metadata/m4a/mp4itunestag.h"
#endif

#include "metabundle.h"

QString MetaBundle::stringStore[STRING_STORE_SIZE];

//if this works, it's because QString is implicitly shared
const QString &MetaBundle::attemptStore( const QString &candidate ) //static
{
    //principal is to cause collisions at reasonable rate to reduce memory
    //consumption while not using such a big store that it is mostly filled with empty QStrings
    //because collisions are so rare

    if( candidate.isEmpty() ) return candidate; //nothing to try to share

    const uchar hash = candidate[0].unicode() % STRING_STORE_SIZE;


    if( stringStore[hash] != candidate ) //then replace
    {
        stringStore[hash] = candidate;
    }

    return stringStore[hash];
}

/// These are untranslated and used for storing/retrieving XML playlist
const QString MetaBundle::columnName( int c ) //static
{
    switch( c ) {
        case Filename:   return "Filename";
        case Title:      return "Title";
        case Artist:     return "Artist";
        case Composer:   return "Composer";
        case Year:       return "Year";
        case Album:      return "Album";
        case DiscNumber: return "DiscNumber";
        case Track:      return "Track";
        case Genre:      return "Genre";
        case Comment:    return "Comment";
        case Directory:  return "Directory";
        case Type:       return "Type";
        case Length:     return "Length";
        case Bitrate:    return "Bitrate";
        case SampleRate: return "SampleRate";
        case Score:      return "Score";
        case Rating:     return "Rating";
        case PlayCount:  return "PlayCount";
        case LastPlayed: return "LastPlayed";
        case Filesize:   return "Filesize";
        case Mood:       return "Mood";
    }
    return "<ERROR>";
}

MetaBundle::MetaBundle()
        : m_year( Undetermined )
        , m_discNumber( Undetermined )
        , m_track( Undetermined )
        , m_bitrate( Undetermined )
        , m_length( Undetermined )
        , m_sampleRate( Undetermined )
        , m_score( Undetermined )
        , m_rating( Undetermined )
        , m_playCount( Undetermined )
        , m_lastPlay( abs( Undetermined ) )
        , m_filesize( Undetermined )
        , m_exists( true )
        , m_isValidMedia( true )
        , m_type( Undetermined )
{
    init();
}

MetaBundle::MetaBundle( const KURL &url, bool noCache, TagLib::AudioProperties::ReadStyle readStyle )
    : m_url( url )
    , m_year( Undetermined )
    , m_discNumber( Undetermined )
    , m_track( Undetermined )
    , m_bitrate( Undetermined )
    , m_length( Undetermined )
    , m_sampleRate( Undetermined )
    , m_score( Undetermined )
    , m_rating( Undetermined )
    , m_playCount( Undetermined )
    , m_lastPlay( abs( Undetermined ) )
    , m_filesize( Undetermined )
    , m_exists( url.protocol() == "file" && QFile::exists( url.path() ) )
    , m_isValidMedia( false ) //will be updated
    , m_type( Undetermined )
{
    if ( m_exists ) {
        if ( !noCache )
            m_isValidMedia = CollectionDB::instance()->bundleForUrl( this );

        if ( !m_isValidMedia || m_length <= 0 )
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
        , m_discNumber( 0 )
        , m_track( 0 )
        , m_bitrate( bitrate )
        , m_length( Irrelevant )
        , m_sampleRate( Unavailable )
        , m_score( Undetermined )
        , m_rating( Undetermined )
        , m_playCount( Undetermined )
        , m_lastPlay( abs( Undetermined ) )
        , m_filesize( Undetermined )
        , m_exists( true )
        , m_isValidMedia( true )
        , m_type( Undetermined )
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

MetaBundle::MetaBundle( QDomNode node )
    : m_year( Undetermined )
    , m_discNumber( Undetermined )
    , m_track( Undetermined )
    , m_bitrate( Undetermined )
    , m_length( Undetermined )
    , m_sampleRate( Undetermined )
    , m_score( Undetermined )
    , m_rating( Undetermined )
    , m_playCount( Undetermined )
    , m_lastPlay( abs( Undetermined ) )
    , m_filesize( Undetermined )
    , m_exists( false )
    , m_isValidMedia( true )
    , m_type( Undetermined )
{
    setUrl( node.toElement().attribute( "url" ) );
    m_exists = isStream() || ( url().protocol() == "file" && QFile::exists( url().path() ) );

    for( uint i = 1, n = NUM_COLUMNS; i < n; ++i )
    {
        switch( i )
        {
            case Artist:
            case Composer:
            case Year:
            case Album:
            case DiscNumber:
            case Track:
            case Title:
            case Genre:
            case Comment:
            case Length:
            case Bitrate:
            case Filesize:
            case SampleRate:
                setExactText( i, node.namedItem( columnName( i ) ).toElement().text() );
                continue;

            default:
                continue;
        }
    }
}

bool
MetaBundle::checkExists()
{
    m_exists = isStream() || ( url().protocol() == "file" && QFile::exists( url().path() ) );

    return m_exists;
}

bool
MetaBundle::operator==( const MetaBundle& bundle ) const
{
    return artist()     == bundle.artist() &&
           title()      == bundle.title() &&
           composer()   == bundle.composer() &&
           album()      == bundle.album() &&
           year()       == bundle.year() &&
           comment()    == bundle.comment() &&
           genre()      == bundle.genre() &&
           track()      == bundle.track() &&
           discNumber() == bundle.discNumber() &&
           length()     == bundle.length() &&
           bitrate()    == bundle.bitrate() &&
           sampleRate() == bundle.sampleRate();
    // FIXME: check for size equality?
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
    if( url().protocol() != "file" )
        return;

    const QString path = url().path();
    TagLib::FileRef fileref;
    TagLib::Tag *tag = 0;


    fileref = TagLib::FileRef( QFile::encodeName( path ), true, readStyle );

    if( !fileref.isNull() ) {
        tag = fileref.tag();

        if ( tag ) {
            #define strip( x ) TStringToQString( x ).stripWhiteSpace()
            setTitle( strip( tag->title() ) );
            setArtist( strip( tag->artist() ) );
            setAlbum( strip( tag->album() ) );
            setComment( strip( tag->comment() ) );
            setGenre( strip( tag->genre() ) );
            setYear( tag->year() );
            setTrack( tag->track() );
            #undef strip

            m_isValidMedia = true;

            m_filesize = QFile( path ).size();
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
                    setComposer( TStringToQString( file->ID3v2Tag()->frameListMap()["TCOM"].front()->toString() ).stripWhiteSpace() );
                }
            }
        }
        else if ( TagLib::Ogg::Vorbis::File *file = dynamic_cast<TagLib::Ogg::Vorbis::File *>( fileref.file() ) ) {
            m_type = ogg;
            if ( file->tag() ) {
                if ( !file->tag()->fieldListMap()[ "COMPOSER" ].isEmpty() ) {
                    setComposer( TStringToQString( file->tag()->fieldListMap()["COMPOSER"].front() ).stripWhiteSpace() );
                }
                if ( !file->tag()->fieldListMap()[ "DISCNUMBER" ].isEmpty() ) {
                    disc = TStringToQString( file->tag()->fieldListMap()["DISCNUMBER"].front() ).stripWhiteSpace();
                }
            }
        }
        else if ( TagLib::MP4::File *file = dynamic_cast<TagLib::MP4::File *>( fileref.file() ) ) {
            m_type = mp4;
            TagLib::MP4::Tag *mp4tag = dynamic_cast<TagLib::MP4::Tag *>( file->tag() );
            if( mp4tag )
            {
                setComposer( TStringToQString( mp4tag->composer() ) );
                disc = QString::number( mp4tag->disk() );
            }
        }
        else
            m_type = other;

        if ( !disc.isEmpty() ) {
            int i = disc.find ('/');
            if ( i != -1 ) {
                setDiscNumber( disc.left( i ).toInt() );
                // disc.right( i ).toInt() is total number of discs, we don't use this at the moment
            }
            else {
                setDiscNumber( disc.toInt() );
            }
        }

        init( fileref.audioProperties() );
    }

    //FIXME disabled for beta4 as it's simpler to not got 100 bug reports
    //else if( KMimeType::findByUrl( m_url )->is( "audio" ) )
    //    init( KFileMetaInfo( m_url, QString::null, KFileMetaInfo::Everything ) );
}

void MetaBundle::updateFilesize()
{
    if( url().protocol() != "file" )
    {
        m_filesize = Undetermined;
        return;
    }

    const QString path = url().path();
    m_filesize = QFile( path ).size();

    debug() << "filesize = " << m_filesize << endl;
}

int MetaBundle::score() const
{
    if( m_score == Undetermined )
        //const_cast is ugly, but other option was mutable, and then we lose const correctness checking
        //everywhere else
        *const_cast<int*>(&m_score) = CollectionDB::instance()->getSongPercentage( m_url.path() );
    return m_score;
}

int MetaBundle::rating() const
{
    if( m_rating == Undetermined )
        *const_cast<int*>(&m_rating) = CollectionDB::instance()->getSongRating( m_url.path() );
    return m_rating;
}

int MetaBundle::playCount() const
{
    if( m_playCount == Undetermined )
        *const_cast<int*>(&m_playCount) = CollectionDB::instance()->getPlayCount( m_url.path() );
    return m_playCount;
}

uint MetaBundle::lastPlay() const
{
    if( (int)m_lastPlay == abs(Undetermined) )
        *const_cast<uint*>(&m_lastPlay) = CollectionDB::instance()->getLastPlay( m_url.path() ).toTime_t();
    return m_lastPlay;
}

void MetaBundle::copy( const MetaBundle &bundle )
{
    setTitle( bundle.title() );
    setArtist( bundle.artist() );
    setComposer( bundle.composer() );
    setAlbum( bundle.album() );
    setYear( bundle.year() );
    setDiscNumber( bundle.discNumber() );
    setComment( bundle.comment() );
    setGenre( bundle.genre() );
    setTrack( bundle.track() );
    setLength( bundle.length() );
    setBitrate( bundle.bitrate() );
    setSampleRate( bundle.sampleRate() );
    setScore( bundle.score() );
    setRating( bundle.rating() );
    setPlayCount( bundle.playCount() );
    setLastPlay( bundle.lastPlay() );
    setFilesize( bundle.filesize() );
}

void MetaBundle::setExactText( int column, const QString &newText )
{
    switch( column )
    {
        case Title:      setTitle(      newText );         break;
        case Artist:     setArtist(     newText );         break;
        case Composer:   setComposer(   newText );         break;
        case Year:       setYear(       newText.toInt() ); break;
        case Album:      setAlbum(      newText );         break;
        case DiscNumber: setDiscNumber( newText.toInt() ); break;
        case Track:      setTrack(      newText.toInt() ); break;
        case Genre:      setGenre(      newText );         break;
        case Comment:    setComment(    newText );         break;
        case Length:     setLength(     newText.toInt() ); break;
        case Bitrate:    setBitrate(    newText.toInt() ); break;
        case SampleRate: setSampleRate( newText.toInt() ); break;
        case Score:      setScore(      newText.toInt() ); break;
        case Rating:     setRating(     newText.toInt() ); break;
        case PlayCount:  setPlayCount(  newText.toInt() ); break;
        case LastPlayed: setLastPlay(   newText.toInt() ); break;
        case Filesize:   setFilesize(   newText.toInt() ); break;
        default: warning() << "Tried to set the text of an immutable or nonexistent column! [" << column << endl;
   }
}

QString MetaBundle::exactText( int column ) const
{
    switch( column )
    {
        case Filename:   return filename();
        case Title:      return title();
        case Artist:     return artist();
        case Composer:   return composer();
        case Year:       return QString::number( year() );
        case Album:      return album();
        case DiscNumber: return QString::number( discNumber() );
        case Track:      return QString::number( track() );
        case Genre:      return genre();
        case Comment:    return comment();
        case Directory:  return directory();
        case Type:       return type();
        case Length:     return QString::number( length() );
        case Bitrate:    return QString::number( bitrate() );
        case SampleRate: return QString::number( sampleRate() );
        case Score:      return QString::number( score() );
        case Rating:     return QString::number( rating() );
        case PlayCount:  return QString::number( playCount() );
        case LastPlayed: return QString::number( lastPlay() );
        case Filesize:   return QString::number( filesize() );
        case Mood:       return QString::null;
        default: warning() << "Tried to get the text of a nonexistent column! [" << column << endl;
    }

    return QString::null; //shouldn't happen
}

QString MetaBundle::prettyText( int column ) const
{
    QString text;
    switch( column )
    {
        case Filename:   text = isStream() ? url().prettyURL() : filename();                         break;
        case Title:      text = title().isEmpty() ? MetaBundle::prettyTitle( filename() ) : title(); break;
        case Artist:     text = artist();                                                            break;
        case Composer:   text = composer();                                                          break;
        case Year:       text = year() ? QString::number( year() ) : QString::null;                  break;
        case Album:      text = album();                                                             break;
        case DiscNumber: text = discNumber() ? QString::number( discNumber() ) : QString::null;      break;
        case Track:      text = track() ? QString::number( track() ) : QString::null;                break;
        case Genre:      text = genre();                                                             break;
        case Comment:    text = comment();                                                           break;
        case Directory:  text = url().isEmpty() ? QString() : directory();                           break;
        case Type:       text = url().isEmpty() ? QString() : type();                                break;
        case Length:     text = prettyLength( length(), true );                                      break;
        case Bitrate:    text = prettyBitrate( bitrate() );                                          break;
        case SampleRate: text = prettySampleRate();                                                  break;
        case Score:      text = QString::number( score() );                                          break;
        case Rating:     text = rating() ? QString::number( rating() ) : QString::null;              break;
        case PlayCount:  text = QString::number( playCount() );                                      break;
        case LastPlayed: text = amaroK::verboseTimeSince( lastPlay() );                              break;
        case Filesize:   text = QString::number( filesize() );                                       break;
        case Mood:       text = QString::null;                                                       break;
        default: warning() << "Tried to get the text of a nonexistent column!" << endl;              break;
    }

    return text.stripWhiteSpace();
}

QString
MetaBundle::prettyTitle() const
{
    QString s = artist();

    //NOTE this gets regressed often, please be careful!
    //     whatever you do, handle the stream case, streams have no artist but have an excellent title

    //FIXME doesn't work for resume playback

    if( !s.isEmpty() ) s += i18n(" - ");
    s += title();
    if( s.isEmpty() ) s = prettyTitle( filename() );
    return s;
}

QString
MetaBundle::veryNiceTitle() const
{
    QString s;
    //NOTE I'm not sure, but the notes and FIXME's in the prettyTitle function should be fixed now.
    //     If not then they do apply to this function also!
    if( !title().isEmpty() )
    {
        if( !artist().isEmpty() )
            s = i18n( "%1 by %2" ).arg( title() ).arg( artist() );
        else
            s = title();
    }
    else
    {
        s = prettyTitle( filename() );
    }
    return s;
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

    switch ( m_type )
    {
        case mp3:
            switch( tag ) {
                case ( composerTag ): id = "TCOM"; break;
                case ( discNumberTag ): id = "TPOS"; break;
            }
            mpegFile = dynamic_cast<TagLib::MPEG::File *>( file );
            if ( mpegFile && mpegFile->ID3v2Tag() )
            {
                if ( value.isEmpty() )
                    mpegFile->ID3v2Tag()->removeFrames( id );
                else
                {
                    if( !mpegFile->ID3v2Tag()->frameListMap()[id].isEmpty() )
                        mpegFile->ID3v2Tag()->frameListMap()[id].front()->setText( QStringToTString( value ) );
                    else {
                        TagLib::ID3v2::TextIdentificationFrame *frame = new TagLib::ID3v2::TextIdentificationFrame( id, TagLib::ID3v2::FrameFactory::instance()->defaultTextEncoding() );
                        frame->setText( QStringToTString( value ) );
                        mpegFile->ID3v2Tag()->addFrame( frame );
                    }
                }
            }
            break;

        case ogg:
            switch( tag ) {
                case ( composerTag ): id = "COMPOSER"; break;
                case ( discNumberTag ): id = "DISCNUMBER"; break;
            }
            oggFile = dynamic_cast<TagLib::Ogg::Vorbis::File *>( file );
            if ( oggFile && oggFile->tag() )
            {
                value.isEmpty() ?
                    oggFile->tag()->removeField( id ):
                    oggFile->tag()->addField( id, QStringToTString( value ), true );
            }
            break;
        case mp4:
            {
                TagLib::MP4::Tag *mp4tag = dynamic_cast<TagLib::MP4::Tag *>( file->tag() );
                if( mp4tag )
                {
                    switch( tag ) {
                        case ( composerTag ): mp4tag->setComposer( QStringToTString( value ) ); break;
                        case ( discNumberTag ): mp4tag->setDisk( value.toInt() );
                    }
                }
            }
    }
}

bool
MetaBundle::save() {
    //Set default codec to UTF-8 (see bugs 111246 and 111232)
    TagLib::ID3v2::FrameFactory::instance()->setDefaultTextEncoding(TagLib::String::UTF8);

    QCString path = QFile::encodeName( url().path() );
    TagLib::FileRef f( path, false );

    if ( !f.isNull() )
    {
        TagLib::Tag * t = f.tag();
        t->setTitle( QStringToTString( title() ) );
        t->setArtist( QStringToTString( artist() ) );
        t->setAlbum( QStringToTString( album() ) );
        t->setTrack( track() );
        t->setYear( year() );
        t->setComment( QStringToTString( comment() ) );
        t->setGenre( QStringToTString( genre() ) );

        if ( hasExtendedMetaInformation() ) {
            setExtendedTag( f.file(), composerTag, composer() );
            setExtendedTag( f.file(), discNumberTag, discNumber() ? QString::number( discNumber() ) : QString() );
        }
        return f.save();
    }
    return false;
}

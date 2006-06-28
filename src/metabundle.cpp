// Max Howell <max.howell@methylblue.com>, (C) 2004
// Alexandre Pereira de Oliveira <aleprj@gmail.com>, (C) 2005
// Gábor Lehel <illissius@gmail.com>, (C) 2005, 2006
// Shane King <kde@dontletsstart.com>, (C) 2006
// Peter C. Ndikuwera <pndiku@gmail.com>, (C) 2006
// License: GNU General Public License V2


#define DEBUG_PREFIX "MetaBundle"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>

#include "amarok.h"
#include "amarokconfig.h"
#include "debug.h"
#include "collectiondb.h"
#include <kfilemetainfo.h>
#include <kio/global.h>
#include <kmdcodec.h>
#include <kmimetype.h>
#include <qdom.h>
#include <qfile.h> //decodePath()
#include <taglib/attachedpictureframe.h>
#include <taglib/fileref.h>
#include <taglib/id3v1genres.h> //used to load genre list
#include <taglib/mpegfile.h>
#include <taglib/tag.h>
#include <taglib/tstring.h>
#include <taglib/id3v2tag.h>
#include <taglib/id3v1tag.h>
#include <taglib/mpegfile.h>
#include <taglib/oggfile.h>
#include <taglib/oggflacfile.h>
#include <taglib/vorbisfile.h>
#include <taglib/flacfile.h>
#include <taglib/textidentificationframe.h>
#include <taglib/uniquefileidentifierframe.h>
#include <taglib/xiphcomment.h>

#include <config.h>
#ifdef HAVE_MP4V2
#include "metadata/mp4/mp4file.h"
#include "metadata/mp4/mp4tag.h"
#else
#include "metadata/m4a/mp4file.h"
#include "metadata/m4a/mp4itunestag.h"
#endif

#include "lastfm.h"
#include "metabundle.h"
#include "podcastbundle.h"

MetaBundle::EmbeddedImage::EmbeddedImage( const TagLib::ByteVector& data, const TagLib::String& description )
    : m_description( TStringToQString( description ) )
{
    m_data.duplicate( data.data(), data.size() );
}

const QCString &MetaBundle::EmbeddedImage::hash() const
{
    if( m_hash.isEmpty() ) {
        m_hash = KMD5( m_data ).hexDigest();
    }
    return m_hash;
}

bool MetaBundle::EmbeddedImage::save( const QDir& dir ) const
{
    QFile   file( dir.filePath( hash() ) );

    if( file.open( IO_WriteOnly | IO_Raw ) ) {
        const Q_LONG s = file.writeBlock( m_data.data(), m_data.size() );
        if( s >= 0 && Q_ULONG( s ) == m_data.size() ) {
            debug() << "EmbeddedImage::save " << file.name() << endl;
            return true;
        }
        file.remove();
    }
    debug() << "EmbeddedImage::save failed! " << file.name() << endl;
    return false;
}

/// These are untranslated and used for storing/retrieving XML playlist
const QString MetaBundle::exactColumnName( int c ) //static
{
    switch( c )
    {
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
    }
    return "<ERROR>";
}

const QString MetaBundle::prettyColumnName( int index ) //static
{
    switch( index )
    {
        case Filename:   return i18n( "Filename"    );
        case Title:      return i18n( "Title"       );
        case Artist:     return i18n( "Artist"      );
        case Composer:   return i18n( "Composer"    );
        case Year:       return i18n( "Year"        );
        case Album:      return i18n( "Album"       );
        case DiscNumber: return i18n( "Disc Number" );
        case Track:      return i18n( "Track"       );
        case Genre:      return i18n( "Genre"       );
        case Comment:    return i18n( "Comment"     );
        case Directory:  return i18n( "Directory"   );
        case Type:       return i18n( "Type"        );
        case Length:     return i18n( "Length"      );
        case Bitrate:    return i18n( "Bitrate"     );
        case SampleRate: return i18n( "Sample Rate" );
        case Score:      return i18n( "Score"       );
        case Rating:     return i18n( "Rating"      );
        case PlayCount:  return i18n( "Play Count"  );
        case LastPlayed: return i18n( "Last Played" );
        case Filesize:   return i18n( "File Size"   );
    }
    return "This is a bug.";
}

int MetaBundle::columnIndex( const QString &name )
{
    for( int i = 0; i < NUM_COLUMNS; ++i )
        if( exactColumnName( i ).lower() == name.lower() )
            return i;
    return -1;
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
        , m_type( other )
        , m_exists( true )
        , m_isValidMedia( true )
        , m_isCompilation( false )
        , m_notCompilation( false )
        , m_podcastBundle( 0 )
        , m_lastFmBundle( 0 )
{
    init();
}

MetaBundle::MetaBundle( const KURL &url, bool noCache, TagLib::AudioProperties::ReadStyle readStyle, EmbeddedImageList* images )
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
    , m_type( other )
    , m_exists( url.protocol() == "file" && QFile::exists( url.path() ) )
    , m_isValidMedia( false )
    , m_isCompilation( false )
    , m_notCompilation( false )
    , m_podcastBundle( 0 )
    , m_lastFmBundle( 0 )
{
    if ( exists() )
    {
        if ( !noCache )
            m_isValidMedia = CollectionDB::instance()->bundleForUrl( this );

        if ( !isValidMedia() || m_length <= 0 )
            readTags( readStyle, images );
    }
    else
    {
        // if it's a podcast we might get some info this way
        CollectionDB::instance()->bundleForUrl( this );
        m_bitrate = m_length = m_sampleRate = Unavailable;
    }
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
        , m_type( other )
        , m_exists( true )
        , m_isValidMedia( false )
        , m_isCompilation( false )
        , m_notCompilation( false )
        , m_podcastBundle( 0 )
        , m_lastFmBundle( 0 )
{
    if( title.contains( '-' ) )
    {
        m_title  = title.section( '-', 1, 1 ).stripWhiteSpace();
        m_artist = title.section( '-', 0, 0 ).stripWhiteSpace();
    }
    else
    {
        m_title  = title;
        m_artist = streamName; //which is sort of correct..
    }
}

MetaBundle::MetaBundle( const MetaBundle &bundle )
{
    *this = bundle;
}

MetaBundle::~MetaBundle()
{
    delete m_podcastBundle;
    delete m_lastFmBundle;
}

MetaBundle&
MetaBundle::operator=( const MetaBundle& bundle )
{
    m_url = bundle.m_url;
    m_title = bundle.m_title;
    m_artist = bundle.m_artist;
    m_composer = bundle.m_composer;
    m_album = bundle.m_album;
    m_comment = bundle.m_comment;
    m_genre = bundle.m_genre;
    m_streamName = bundle.m_streamName;
    m_streamUrl = bundle.m_streamUrl;
    m_uniqueId = bundle.m_uniqueId;
    m_year = bundle.m_year;
    m_discNumber = bundle.m_discNumber;
    m_track = bundle.m_track;
    m_bitrate = bundle.m_bitrate;
    m_length = bundle.m_length;
    m_sampleRate = bundle.m_sampleRate;
    m_score = bundle.m_score;
    m_rating = bundle.m_rating;
    m_playCount = bundle.m_playCount;
    m_lastPlay = bundle.m_lastPlay;
    m_filesize = bundle.m_filesize;
    m_type = bundle.m_type;
    m_exists = bundle.m_exists;
    m_isValidMedia = bundle.m_isValidMedia;
    m_isCompilation = bundle.m_isCompilation;
    m_notCompilation = bundle.m_notCompilation;

//    delete m_podcastBundle; why does this crash Amarok? apparently m_podcastBundle isn't always initialized.
    m_podcastBundle = 0;
    if( bundle.m_podcastBundle )
        setPodcastBundle( *bundle.m_podcastBundle );

//    delete m_lastFmBundle; same as above
    m_lastFmBundle = 0;
    if( bundle.m_lastFmBundle )
        setLastFmBundle( *bundle.m_lastFmBundle );


    return *this;
}


bool
MetaBundle::checkExists()
{
    m_exists = isStream() || url().protocol() == "cdda" || ( url().protocol() == "file" && QFile::exists( url().path() ) );

    return m_exists;
}

bool
MetaBundle::operator==( const MetaBundle& bundle ) const
{
    return uniqueId()   == bundle.uniqueId() && //first, since if using IDs will return faster
           artist()     == bundle.artist() &&
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
MetaBundle::clear()
{
    *this = MetaBundle();
}

void
MetaBundle::init( TagLib::AudioProperties *ap )
{
    if ( ap )
    {
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
        const KFileMetaInfoItem itemtitle = info.item( "Title" );
        m_title = itemtitle.isValid() ? itemtitle.string() : prettyTitle( m_url.fileName() );

        const KFileMetaInfoItem itemid = info.item( "Unique ID" );
        m_uniqueId = itemid.isValid() ? itemid.string() : QString::null;

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
    else
    {
        m_bitrate = m_length = m_sampleRate = m_filesize = Undetermined;
        m_isValidMedia = false;
    }
}

void
MetaBundle::embeddedImages( MetaBundle::EmbeddedImageList& images )
{
    if ( url().protocol() == "file" ) {
        TagLib::FileRef fileref = TagLib::FileRef( QFile::encodeName( url().path() ), false );
        if ( !fileref.isNull() ) {
            if ( TagLib::MPEG::File *file = dynamic_cast<TagLib::MPEG::File *>( fileref.file() ) ) {
                if ( file->ID3v2Tag() )
                    loadImagesFromTag( *file->ID3v2Tag(), images );
            } else if ( TagLib::FLAC::File *file = dynamic_cast<TagLib::FLAC::File *>( fileref.file() ) ) {
                if ( file->ID3v2Tag() )
                    loadImagesFromTag( *file->ID3v2Tag(), images );
            } else if ( TagLib::MP4::File *file = dynamic_cast<TagLib::MP4::File *>( fileref.file() ) ) {
                TagLib::MP4::Tag *mp4tag = dynamic_cast<TagLib::MP4::Tag *>( file->tag() );
                if( mp4tag && mp4tag->cover().size() ) {
                    images.push_back( EmbeddedImage( mp4tag->cover(), "" ) );
                }
            }
        }
    }
}

void
MetaBundle::readTags( TagLib::AudioProperties::ReadStyle readStyle, EmbeddedImageList* images )
{
    if(!( url().protocol() == "file" || url().protocol() == "audiocd" ) )
        return;

    const QString path = url().path();
    TagLib::FileRef fileref;
    TagLib::Tag *tag = 0;


    fileref = TagLib::FileRef( QFile::encodeName( path ), true, readStyle );

    if( !fileref.isNull() )
    {
        tag = fileref.tag();

        if ( tag )
        {
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

        QString disc;
        QString compilation;
        bool atf = AmarokConfig::advancedTagFeatures();
        if ( TagLib::MPEG::File *file = dynamic_cast<TagLib::MPEG::File *>( fileref.file() ) )
        {
            m_type = mp3;
            if ( file->ID3v2Tag( atf ) )
            {
                if ( !file->ID3v2Tag()->frameListMap()["TPOS"].isEmpty() )
                    disc = TStringToQString( file->ID3v2Tag()->frameListMap()["TPOS"].front()->toString() ).stripWhiteSpace();

                if ( !file->ID3v2Tag()->frameListMap()["TCOM"].isEmpty() )
                    setComposer( TStringToQString( file->ID3v2Tag()->frameListMap()["TCOM"].front()->toString() ).stripWhiteSpace() );

                if ( !file->ID3v2Tag()->frameListMap()["TCMP"].isEmpty() )
                    compilation = TStringToQString( file->ID3v2Tag()->frameListMap()["TCMP"].front()->toString() ).stripWhiteSpace();

                if(images) {
                    loadImagesFromTag( *file->ID3v2Tag(), *images );
                }
            }
        }
        else if ( TagLib::Ogg::Vorbis::File *file = dynamic_cast<TagLib::Ogg::Vorbis::File *>( fileref.file() ) )
        {
            m_type = ogg;
            if ( file->tag() )
            {
                if ( !file->tag()->fieldListMap()[ "COMPOSER" ].isEmpty() )
                    setComposer( TStringToQString( file->tag()->fieldListMap()["COMPOSER"].front() ).stripWhiteSpace() );

                if ( !file->tag()->fieldListMap()[ "DISCNUMBER" ].isEmpty() )
                    disc = TStringToQString( file->tag()->fieldListMap()["DISCNUMBER"].front() ).stripWhiteSpace();

                if ( !file->tag()->fieldListMap()[ "COMPILATION" ].isEmpty() )
                    compilation = TStringToQString( file->tag()->fieldListMap()["COMPILATION"].front() ).stripWhiteSpace();
            }
        }
        else if ( TagLib::FLAC::File *file = dynamic_cast<TagLib::FLAC::File *>( fileref.file() ) )
        {
            m_type = flac;
            if ( file->xiphComment( atf ) )
            {
                if ( !file->xiphComment()->fieldListMap()[ "COMPOSER" ].isEmpty() )
                    setComposer( TStringToQString( file->xiphComment()->fieldListMap()["COMPOSER"].front() ).stripWhiteSpace() );

                if ( !file->xiphComment()->fieldListMap()[ "DISCNUMBER" ].isEmpty() )
                    disc = TStringToQString( file->xiphComment()->fieldListMap()["DISCNUMBER"].front() ).stripWhiteSpace();

                if ( !file->xiphComment()->fieldListMap()[ "COMPILATION" ].isEmpty() )
                    compilation = TStringToQString( file->xiphComment()->fieldListMap()["COMPILATION"].front() ).stripWhiteSpace();
            }

            if ( images && file->ID3v2Tag() ) {
                loadImagesFromTag( *file->ID3v2Tag(), *images );
            }
        }
        else if ( TagLib::MP4::File *file = dynamic_cast<TagLib::MP4::File *>( fileref.file() ) )
        {
            m_type = mp4;
            TagLib::MP4::Tag *mp4tag = dynamic_cast<TagLib::MP4::Tag *>( file->tag() );
            if( mp4tag )
            {
                setComposer( TStringToQString( mp4tag->composer() ) );
                disc = QString::number( mp4tag->disk() );
                compilation = QString::number( mp4tag->compilation() );
                if ( images && mp4tag->cover().size() ) {
                    images->push_back( EmbeddedImage( mp4tag->cover(), "" ) );
                }
            }
        }

        m_uniqueId = QString::null;
        if ( atf )
            setUniqueId( fileref );

        if ( !disc.isEmpty() )
        {
            int i = disc.find ('/');
            if ( i != -1 )
                // disc.right( i ).toInt() is total number of discs, we don't use this at the moment
                setDiscNumber( disc.left( i ).toInt() );
            else
                setDiscNumber( disc.toInt() );
        }

        if ( compilation.isEmpty() ) {
            // well, it wasn't set, but if the artist is VA assume it's a compilation
            if ( artist().string() == i18n( "Various Artists" ) )
                setCompilation( CompilationYes );
        } else {
            int i = compilation.toInt();
            if ( i == CompilationNo )
                setCompilation( CompilationNo );
            else if ( i == CompilationYes )
                setCompilation( CompilationYes );
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

void MetaBundle::copyFrom( const MetaBundle &bundle )
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
    setFileType( bundle.fileType() );
    setFilesize( bundle.filesize() );
    if( bundle.m_podcastBundle )
        setPodcastBundle( *bundle.m_podcastBundle );
    else
    {
        delete m_podcastBundle;
        m_podcastBundle = 0;
    }

    if( bundle.m_lastFmBundle )
        setLastFmBundle( *bundle.m_lastFmBundle );
    else
    {
        delete m_lastFmBundle;
        m_lastFmBundle = 0;
    }
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
        case Type:       setFileType(   newText.toInt() ); break;
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
        case Type:       return QString::number( fileType() );
        case Length:     return QString::number( length() );
        case Bitrate:    return QString::number( bitrate() );
        case SampleRate: return QString::number( sampleRate() );
        case Score:      return QString::number( score() );
        case Rating:     return QString::number( rating() );
        case PlayCount:  return QString::number( playCount() );
        case LastPlayed: return QString::number( lastPlay() );
        case Filesize:   return QString::number( filesize() );
        default: warning() << "Tried to get the text of a nonexistent column! [" << column << endl;
    }

    return QString::null; //shouldn't happen
}

QString MetaBundle::prettyText( int column ) const
{
    QString text;
    switch( column )
    {
        case Filename:   text = isStream() ? url().prettyURL():MetaBundle::prettyTitle(filename());  break;
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
        case Rating:     text = prettyRating();                                                      break;
        case PlayCount:  text = QString::number( playCount() );                                      break;
        case LastPlayed: text = amaroK::verboseTimeSince( lastPlay() );                              break;
        case Filesize:   text = prettyFilesize();                                                    break;
        default: warning() << "Tried to get the text of a nonexistent column!" << endl;              break;
    }

    return text.stripWhiteSpace();
}

bool MetaBundle::matchesSimpleExpression( const QString &expression, const QValueList<int> &columns ) const
{
    const QStringList terms = QStringList::split( ' ', expression.lower() );
    bool matches = true;
    for( uint x = 0; matches && x < terms.count(); ++x )
    {
        uint y = 0, n = columns.count();
        for(; y < n; ++y )
            if ( prettyText( columns[y] ).lower().contains( terms[x] ) )
                break;
        matches = ( y < n );
    }

    return matches;
}

bool MetaBundle::matchesExpression( const QString &expression, const QValueList<int> &defaultColumns ) const
{
    return matchesParsedExpression( ExpressionParser::parse( expression ), defaultColumns );
}

bool MetaBundle::matchesParsedExpression( const ParsedExpression &data, const QValueList<int> &defaults ) const
{
    for( uint i = 0, n = data.count(); i < n; ++i ) //check each part for matchiness
    {
        bool b = false; //whether at least one matches
        for( uint ii = 0, count = data[i].count(); ii < count; ++ii )
        {
            expression_element e = data[i][ii];
            int column = -1;
            if( !e.field.isEmpty() )
                column = columnIndex( e.field.lower() );
            if( column >= 0 ) //a field was specified and it exists
            {
                QString q = e.text, v = prettyText( column ).lower(), w = q.lower();
                //q = query, v = contents of the field, w = match against it
                bool condition; //whether it matches, not taking e.negateation into account

                bool numeric;
                switch( column )
                {
                    case Year:
                    case DiscNumber:
                    case Track:
                    case Bitrate:
                    case SampleRate:
                    case Score:
                    case PlayCount:
                    case LastPlayed:
                    case Filesize:
                        numeric = true;
                        break;
                    default:
                        numeric = false;
                }

                if( column == Filesize )
                {
                    v = QString::number( filesize() );
                    if( w.endsWith( "m" ) )
                        w = QString::number( w.left( w.length()-1 ).toLong() * 1024 * 1024 );
                    else if( w.endsWith( "k" ) )
                        w = QString::number( w.left( w.length()-1 ).toLong() * 1024 );
                }

                if( e.match == expression_element::More )
                {
                    if( numeric )
                        condition = v.toInt() > w.toInt();
                    else if( column == Rating )
                        condition = v.toFloat() > w.toFloat();
                    else if( column == Length )
                    {
                        int g = v.find( ':' ), h = w.find( ':' );
                        condition = v.left( g ).toInt() > w.left( h ).toInt() ||
                                    ( v.left( g ).toInt() == w.left( h ).toInt() &&
                                      v.mid( g + 1 ).toInt() > w.mid( h + 1 ).toInt() );
                    }
                    else
                        condition = v > w; //compare the strings
                }
                else if( e.match == expression_element::Less )
                {
                    if( numeric )
                        condition = v.toInt() < w.toInt();
                    else if( column == Rating )
                        condition = v.toFloat() < w.toFloat();
                    else if( column == Length )
                    {
                        int g = v.find( ':' ), h = w.find( ':' );
                        condition = v.left( g ).toInt() < w.left( h ).toInt() ||
                                    ( v.left( g ).toInt() == w.left( h ).toInt() &&
                                      v.mid( g + 1 ).toInt() < w.mid( h + 1 ).toInt() );
                    }
                    else
                        condition = v < w;
                }
                else
                {
                    if( numeric )
                        condition = v.toInt() == w.toInt();
                    else if( column == Rating )
                        condition = v.toFloat() == w.toFloat();
                    else if( column == Length )
                    {
                        int g = v.find( ':' ), h = w.find( ':' );
                        condition = v.left( g ).toInt() == w.left( h ).toInt() &&
                                    v.mid( g + 1 ).toInt() == w.mid( h + 1 ).toInt();
                    }
                    else
                        condition = v.contains( q, false );
                }
                if( condition == ( e.negate ? false : true ) )
                {
                    b = true;
                    break;
                }
            }
            else //check just the default fields
            {
                for( int it = 0, end = defaults.size(); it != end; ++it )
                {
                    b = prettyText( defaults[it] ).contains( e.text, false ) == ( e.negate ? false : true );
                    if( ( e.negate && !b ) || ( !e.negate && b ) )
                        break;
                }
                if( b )
                    break;
            }
        }
        if( !b )
            return false;
    }

    return true;
}

QString
MetaBundle::prettyTitle() const
{
    QString s = artist();

    //NOTE this gets regressed often, please be careful!
    //     whatever you do, handle the stream case, streams have no artist but have an excellent title

    //FIXME doesn't work for resume playback

    if( s.isEmpty() )
        s = title();
    else
        s = i18n("%1 - %2").arg( artist(), title() );

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
            s = i18n( "%1 by %2" ).arg( title(), artist() );
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

    //remove .part extension if it exists
    if (s.endsWith( ".part" ))
        s = s.left( s.length() - 5 );

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
MetaBundle::veryPrettyTime( int time )
{
    if( time == Undetermined )
        return i18n( "?" );
    if( time == Irrelevant )
        return i18n( "-" );

    QStringList s;
    s << QString::number( time % 60 ); //seconds
    time /= 60;
    if( time )
        s << QString::number( time % 60 ); //minutes
    time /= 60;
    if( time )
        s << QString::number( time % 24 ); //hours
    time /= 24;
    if( time )
        s << QString::number( time ); //days

    switch( s.count() )
    {
        case 1: return i18n( "seconds", "%1s" ).arg( s[0] );
        case 2: return i18n( "minutes, seconds", "%2m %1s" ).arg( s[0], s[1] );
        case 3: return i18n( "hours, minutes, seconds", "%3h %2m %1s" ).arg( s[0], s[1], s[2] );
        case 4: return i18n( "days, hours, minutes, seconds", "%4d %3h %2m %1s" ).arg( s[0], s[1], s[2], s[3] );
        default: return "omg bug!";
    }
}

QString
MetaBundle::fuzzyTime( int time )
{
    QString s;
    int secs=0, min=0, hr=0, day=0, week=0;

    if( time == Undetermined )
        return i18n( "?" );
    if( time == Irrelevant )
        return i18n( "-" );

    secs = time % 60; //seconds
    time /= 60;
    if( time )
        min = time % 60; //minutes
    time /= 60;
    if( time )
        hr = time % 24 ; //hours
    time /= 24;
    if( time )
        day = time ; //days
    time /= 7;
    if( time )
        week = time ; //weeks

    if ( week )
        return i18n( "%1 weeks" ).arg( QString::number( week + (float( day ) / 7), 'f', 1 ));
    else if ( day )
        return i18n( "%1 days" ).arg( QString::number( day + (float( hr ) / 24), 'f', 1 ));
    else if ( hr )
        return i18n( "%1 hours" ).arg( QString::number( hr + (float( min ) / 60), 'f', 1 ));
    else
        return i18n( "%1:%2").arg( min ).arg( zeroPad( secs ) );
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

QString
MetaBundle::prettyFilesize( int s )
{
    return KIO::convertSize( s );
}

QString
MetaBundle::prettyRating( int r, bool trailingzero ) //static
{
    if( trailingzero )
        return QString::number( float( r ) / 2, 'f', 1 );
    else
        return r ? QString::number( float( r ) / 2 ) : QString();
}

QString
MetaBundle::ratingDescription( int r )
{
    switch( r )
    {
        case 2: return i18n( "Awful" );
        case 3: return i18n( "Barely tolerable" );
        case 4: return i18n( "Tolerable" );
        case 5: return i18n( "Okay" );
        case 6: return i18n( "Good" );
        case 7: return i18n( "Very good" );
        case 8: return i18n( "Excellent" );
        case 9: return i18n( "Amazing" );
        case 10: return i18n( "Favorite" );
        case 0: default: return i18n( "Not rated" ); // assume weird values as not rated
    }
    return "if you can see this, then that's a bad sign.";
}

QStringList
MetaBundle::ratingList()
{
    QString s = i18n( "rating - description", "%1 - %2" );
    QStringList list;
    list += ratingDescription( 0 );
    for ( int i = 2; i<=10; i++ )
        list += s.arg( prettyRating( i, true ) ).arg( ratingDescription( i ) );
    return list;
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
MetaBundle::setExtendedTag( TagLib::File *file, int tag, const QString value )
{
    char *id = 0;

    if ( m_type == mp3 )
    {
        switch( tag )
        {
            case ( composerTag ): id = "TCOM"; break;
            case ( discNumberTag ): id = "TPOS"; break;
            case ( compilationTag ): id = "TCMP"; break;
        }
        TagLib::MPEG::File *mpegFile = dynamic_cast<TagLib::MPEG::File *>( file );
        if ( mpegFile && mpegFile->ID3v2Tag() )
        {
            if ( value.isEmpty() )
                mpegFile->ID3v2Tag()->removeFrames( id );
            else
            {
                if( !mpegFile->ID3v2Tag()->frameListMap()[id].isEmpty() )
                    mpegFile->ID3v2Tag()->frameListMap()[id].front()->setText( QStringToTString( value ) );
                else
                {
                    TagLib::ID3v2::TextIdentificationFrame *frame = new TagLib::ID3v2::TextIdentificationFrame( id, TagLib::ID3v2::FrameFactory::instance()->defaultTextEncoding() );
                    frame->setText( QStringToTString( value ) );
                    mpegFile->ID3v2Tag()->addFrame( frame );
                }
            }
        }
    }
    else if ( m_type == ogg )
    {
        switch( tag )
        {
            case ( composerTag ): id = "COMPOSER"; break;
            case ( discNumberTag ): id = "DISCNUMBER"; break;
            case ( compilationTag ): id = "COMPILATION"; break;
        }
        TagLib::Ogg::Vorbis::File *oggFile = dynamic_cast<TagLib::Ogg::Vorbis::File *>( file );
        if ( oggFile && oggFile->tag() )
        {
            value.isEmpty() ?
                oggFile->tag()->removeField( id ):
                oggFile->tag()->addField( id, QStringToTString( value ), true );
        }
    }
    else if ( m_type == flac )
    {
        switch( tag )
        {
            case ( composerTag ): id = "COMPOSER"; break;
            case ( discNumberTag ): id = "DISCNUMBER"; break;
            case ( compilationTag ): id = "COMPILATION"; break;
        }
        TagLib::FLAC::File *flacFile = dynamic_cast<TagLib::FLAC::File *>( file );
        if ( flacFile && flacFile->xiphComment() )
        {
            value.isEmpty() ?
            flacFile->xiphComment()->removeField( id ):
            flacFile->xiphComment()->addField( id, QStringToTString( value ), true );
        }
    }
    else if ( m_type == mp4 )
    {
        TagLib::MP4::Tag *mp4tag = dynamic_cast<TagLib::MP4::Tag *>( file->tag() );
        if( mp4tag )
        {
            switch( tag )
            {
                case ( composerTag ): mp4tag->setComposer( QStringToTString( value ) ); break;
                case ( discNumberTag ): mp4tag->setDisk( value.toInt() );
                case ( compilationTag ): mp4tag->setCompilation( value.toInt() == CompilationYes );
            }
        }
    }
}

void
MetaBundle::setPodcastBundle( const PodcastEpisodeBundle &peb )
{
    delete m_podcastBundle;
    m_podcastBundle = new PodcastEpisodeBundle;
    *m_podcastBundle = peb;
}

void
MetaBundle::setLastFmBundle( const LastFm::Bundle &last )
{
    delete m_lastFmBundle;
   // m_lastFmBundle = new LastFm::Bundle(last);
   m_lastFmBundle = new LastFm::Bundle;
   *m_lastFmBundle = last;
}

void MetaBundle::loadImagesFromTag( const TagLib::ID3v2::Tag &tag, EmbeddedImageList& images )
{
    TagLib::ID3v2::FrameList l = tag.frameListMap()[ "APIC" ];
    foreachType( TagLib::ID3v2::FrameList, l ) {
        debug() << "Found APIC frame" << endl;
        TagLib::ID3v2::AttachedPictureFrame *ap = static_cast<TagLib::ID3v2::AttachedPictureFrame*>( *it );

        const TagLib::ByteVector &imgVector = ap->picture();
        debug() << "Size of image: " <<  imgVector.size() << " byte" << endl;
        // ignore APIC frames without picture and those with obviously bogus size
        if( imgVector.size() > 0 && imgVector.size() < 10000000 /*10MB*/ ) {
            images.push_back( EmbeddedImage( imgVector, ap->description() ) );
        }
    }
}

bool
MetaBundle::save()
{
    //Set default codec to UTF-8 (see bugs 111246 and 111232)
    TagLib::ID3v2::FrameFactory::instance()->setDefaultTextEncoding(TagLib::String::UTF8);

    QCString path = QFile::encodeName( url().path() );
    TagLib::FileRef f( path, false );

    if ( !f.isNull() )
    {
        TagLib::Tag * t = f.tag();
        if ( t ) { // f.tag() can return null if the file couldn't be opened for writing
            t->setTitle( QStringToTString( title() ) );
            t->setArtist( QStringToTString( artist().string() ) );
            t->setAlbum( QStringToTString( album().string() ) );
            t->setTrack( track() );
            t->setYear( year() );
            t->setComment( QStringToTString( comment().string() ) );
            t->setGenre( QStringToTString( genre().string() ) );

            if ( hasExtendedMetaInformation() )
            {
                setExtendedTag( f.file(), composerTag, composer() );
                setExtendedTag( f.file(), discNumberTag, discNumber() ? QString::number( discNumber() ) : QString() );
                if ( compilation() != CompilationUnknown )
                    setExtendedTag( f.file(), compilationTag, QString::number( compilation() ) );
            }
            return f.save();
        }
    }
    return false;
}

bool MetaBundle::save( QTextStream &stream, const QStringList &attributes, int indent ) const
{
    QDomDocument QDomSucksItNeedsADocument;
    QDomElement item = QDomSucksItNeedsADocument.createElement( "item" );
    item.setAttribute( "url", url().url() );
    item.setAttribute( "uniqueid", uniqueId() );
    if( m_isCompilation )
        item.setAttribute( "compilation", "true" );

    for( int i = 0, n = attributes.count(); i < n; i += 2 )
        item.setAttribute( attributes[i], attributes[i+1] );

    for( int i = 0; i < NUM_COLUMNS; ++i )
    {
        QDomElement tag = QDomSucksItNeedsADocument.createElement( exactColumnName( i ) );
        //debug() << "exactColumName(i) = " << exactColumnName( i ) << endl;
        QDomText text = QDomSucksItNeedsADocument.createTextNode( exactText( i ) );
        //debug() << "exactText(i) = " << exactText( i ) << endl;
        tag.appendChild( text );

        item.appendChild( tag );
    }

    item.save( stream, indent );
    return true;
}

void MetaBundle::setUrl( const KURL &url )
{
    QValueList<int> changes;
    for( int i = 0; i < NUM_COLUMNS; ++i ) changes << i;
    aboutToChange( changes ); m_url = url; reactToChanges( changes );
}

void MetaBundle::setPath( const QString &path )
{
    QValueList<int> changes;
    for( int i = 0; i < NUM_COLUMNS; ++i ) changes << i;
    aboutToChange( changes ); m_url.setPath( path ); reactToChanges( changes );
}

void MetaBundle::setUniqueId()
{
    const QString path = url().path();
    TagLib::FileRef fileref;
    fileref = TagLib::FileRef( QFile::encodeName( path ), true, TagLib::AudioProperties::Fast );

    if( !fileref.isNull() )
        setUniqueId( fileref );
}

void MetaBundle::setUniqueId( const QString &id )
{
    //WARNING WARNING WARNING
    //NEVER CALL THIS FUNCTION UNLESS YOU'RE DAMN SURE YOU KNOW WHAT YOU ARE DOING
    //i.e. IF YOU ARE NOT JEFF, ASK HIM BEFORE DOING THIS! YOU CAN CAUSE BAD
    //THINGS TO HAPPEN LIKE MESSING UP USERS' COLLECTIONS, WHICH THEY ARE NOT
    //VERY FORGIVING ABOUT
    m_uniqueId = id;
}

void MetaBundle::setUniqueId( TagLib::FileRef &fileref, bool recreate )
{
    //DEBUG_BLOCK

    if( !AmarokConfig::advancedTagFeatures() )
        return;

    int createID = 0;
    int randSize = 8; //largest size allowed by ID3v2.4
    bool newID = false;

    QString ourId = QString( "Amarok - rediscover your music at http://amarok.kde.org" ).upper();

    if ( TagLib::MPEG::File *file = dynamic_cast<TagLib::MPEG::File *>( fileref.file() ) )
    {
        if ( file->ID3v2Tag( true ) )
        {
            if( file->ID3v2Tag()->frameListMap()["UFID"].isEmpty() )
                createID = 1;
            else
            {
                TagLib::ID3v2::UniqueFileIdentifierFrame* ufidf =
                        dynamic_cast<TagLib::ID3v2::UniqueFileIdentifierFrame*>(
                                    file->ID3v2Tag()->frameListMap()["UFID"].front() );
                if( TStringToQString( ufidf->owner() ) != ourId || recreate )
                {
                    file->ID3v2Tag()->removeFrames( "UFID" );
                    createID = 1;
                }
                else
                    //this is really ugly, but otherwise we get an incorrect ? at the end of the string...possibly a null value?  Not sure of another way to fix this.
                    m_uniqueId = TStringToQString( TagLib::String( ufidf->identifier().data() ) ).left( randSize );
            }
            if( createID == 1 && TagLib::File::isWritable( file->name() ) )
            {
                m_uniqueId = getRandomStringHelper( randSize );
                file->ID3v2Tag()->addFrame( new TagLib::ID3v2::UniqueFileIdentifierFrame(
                            QStringToTString( ourId ),
                            TagLib::ByteVector( m_uniqueId.ascii(), randSize )
                            ) );
                file->save( TagLib::MPEG::File::ID3v2 );
                newID = true;
            }
        }
    }
    else if ( TagLib::Ogg::Vorbis::File *file = dynamic_cast<TagLib::Ogg::Vorbis::File *>( fileref.file() ) )
    {
        if( file->tag() )
        {
            if( file->tag()->fieldListMap().contains( QStringToTString( ourId ) ) && recreate )
                file->tag()->removeField( QStringToTString( ourId ) );
            if( !file->tag()->fieldListMap().contains( QStringToTString( ourId ) ) )
            {
                if( TagLib::File::isWritable( file->name() ) )
                {
                    m_uniqueId = getRandomStringHelper( randSize );
                    file->tag()->addField( QStringToTString( ourId ),
                            TagLib::ByteVector( m_uniqueId.ascii(), randSize )
                            );
                    file->save();
                    newID = true;
                }
            }
            else
            {
                m_uniqueId = TStringToQString( file->tag()->fieldListMap()[QStringToTString( ourId )].front() ).left( randSize );
            }
        }
    }
    //FLACs are commented because supposedly they have to be totally rewritten to add uniqueids, and as they're big this takes a while.
    //This will have to be tested.  It's also supposedly a TagLib bug so maybe it could be fixed, especially with some pressure on wheels
    else if ( TagLib::FLAC::File *file = dynamic_cast<TagLib::FLAC::File *>( fileref.file() ) )
    {
        /*if ( file->xiphComment( true ) )
        {
            if( file->xiphComment()->fieldListMap().contains( QStringToTString( ourId ) ) && recreate )
                file->xiphComment()->removeField( QStringToTString( ourId ) );
            if( !file->xiphComment()->fieldListMap().contains( QStringToTString( ourId ) ) )
            {
                if( TagLib::File::isWritable( file->name() ) )
                {
                    m_uniqueId = getRandomStringHelper( randSize );
                    file->xiphComment()->addField( QStringToTString( ourId ),
                            TagLib::ByteVector( m_uniqueId.ascii(), randSize )
                            );
                    file->save();
                    newID = true;
                }
            }
            else
                m_uniqueId = TStringToQString( file->xiphComment()->fieldListMap()[QStringToTString( ourId )].front() ).left( randSize );
        }*/
        //don't handle FLAC yet because causes whole file to be rewritten -- bug in TagLib?
        if( file || !file )
            return;
    }
    else if ( TagLib::Ogg::FLAC::File *file = dynamic_cast<TagLib::Ogg::FLAC::File *>( fileref.file() ) )
    {
        /*if( file->tag() )
        {
            if( file->tag()->fieldListMap().contains( QStringToTString( ourId ) ) && recreate )
                file->tag()->removeField( QStringToTString( ourId ) );
            if( !file->tag()->fieldListMap().contains( QStringToTString( ourId ) ) )
            {
                if( TagLib::File::isWritable( file->name() ) )
                {
                    m_uniqueId = getRandomStringHelper( randSize );
                    file->tag()->addField( QStringToTString( ourId ),
                            TagLib::ByteVector( m_uniqueId.ascii(), randSize )
                            );
                    file->save();
                    newID = true;
                }
            }
            else
                m_uniqueId = TStringToQString( file->tag()->fieldListMap()[QStringToTString( ourId )].front() ).left( randSize );
        }*/
        if( file || !file )
            return;
    }
    else if ( TagLib::MP4::File *file = dynamic_cast<TagLib::MP4::File *>( fileref.file() ) )
    {
        if( file || !file )
            return; //not handled, at least not yet
    }
    //debug() << "Unique id for file = " << fileref.file()->name() << " is " << m_uniqueId << " and this " << (newID ? "IS" : "is NOT" ) << " a new unique id." << endl;
}

void
MetaBundle::newUniqueId()
{
    //DEBUG_BLOCK
    const QString path = url().path();
    TagLib::FileRef fileref;
    fileref = TagLib::FileRef( QFile::encodeName( path ), true, TagLib::AudioProperties::Fast );

    if( !fileref.isNull() )
        setUniqueId( fileref, true );
    else
        debug() << "ERROR: failed to set new uniqueid (could not open fileref)" << endl;
}

int
MetaBundle::getRand()
{
    //KRandom  supposedly exists in SVN, although it's not checked out on my machine, and it's certainly not in 3.3, so I'm just going to steal its code

    unsigned int seed;
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0 || ::read(fd, &seed, sizeof(seed)) != sizeof(seed))
    {
            // No /dev/urandom... try something else.
            srand(getpid());
            seed = rand()+time(0);
    }
    if (fd >= 0) close(fd);
    srand(seed);
    return rand();
}

QString
MetaBundle::getRandomString( int size )
{
    if( size != 8 )
    {
        debug() << "Wrong size passed in!" << endl;
        return QString::null;
    }

    QString str;
    //do a memory op once, much faster than doing multiple later, especially since we know how big it will be
    str.reserve( size );
    int i = getRand(); //seed it
    i = 0;
    while (size--)
    {
        // check your ASCII tables
        // we want characters you can see...93 is the range from ! to ~
        int r=rand() % 93;
        // shift the value to the visible characters
        r+=33;
        // we don't want ", %, ', <, >, \, or `
        // so that we don't have issues with escaping/quoting in QStrings,
        // and so that we don't have <> in our XML files where they might cause issues
        // hopefully this list is final, as once users really start using this
        // it will be a pain to change...however, there is an ATF version in CollectionDB
        // which will help if this ever needs to change
        // In addition we can change our vendor string
        if (r==34 || r==37 || r==39 || r==60 ||r == 62 || r==92 || r==96) r+=1;
        str[i++] =  char(r);
        // this next comment kept in for fun, as it was from the source of KRandomString, where I got
        // most of this code from to start with :-)
        // so what if I work backwards?
    }
    return str;
}

QString
MetaBundle::getRandomStringHelper( int size )
{
    QString returnvalue;
    bool goodvalue = false;
    bool temporary = false;
    QStringList tempcheck, uniqueids;
    if (CollectionDB::instance()->getType() == DbConnection::postgresql)
        tempcheck = CollectionDB::instance()->query( QString( "select relname from pg_stat_user_tables order by relname;" ) );
    else
        tempcheck = CollectionDB::instance()->query( QString( "SHOW TABLES;" ) );

    if( tempcheck.contains( "uniqueid_temp" ) > 0 )
        temporary = true;
    while( !goodvalue )
    {
        returnvalue = getRandomString( size );
        uniqueids = CollectionDB::instance()->query( QString(
            "SELECT url, uniqueid "
            "FROM uniqueid%1 "
            "WHERE uniqueid = '%2';" )
                .arg( temporary ? "_temp" : "" )
                .arg( CollectionDB::instance()->escapeString( returnvalue ) ) );
        if( uniqueids.count() == 0 )
            goodvalue = true;
    }
    return returnvalue;
}

void MetaBundle::setTitle( const QString &title )
{ aboutToChange( Title ); m_title = title; reactToChange( Title ); }

void MetaBundle::setArtist( const AtomicString &artist )
{ aboutToChange( Artist ); m_artist = artist; reactToChange( Artist ); }

void MetaBundle::setAlbum( const AtomicString &album )
{ aboutToChange( Album ); m_album = album; reactToChange( Album ); }

void MetaBundle::setComment( const AtomicString &comment )
{ aboutToChange( Comment ); m_comment = comment; reactToChange( Comment ); }

void MetaBundle::setGenre( const AtomicString &genre )
{ aboutToChange( Genre ); m_genre = genre; reactToChange( Genre ); }

void MetaBundle::setYear( int year)
{ aboutToChange( Year ); m_year = year; reactToChange( Year ); }

void MetaBundle::setTrack( int track )
{ aboutToChange( Track ); m_track = track; reactToChange( Track ); }

void MetaBundle::setCompilation( int compilation )
{
    switch( compilation )
    {
        case CompilationYes:
            m_isCompilation = true;
            m_notCompilation = false;
            break;
        case CompilationNo:
            m_isCompilation = false;
            m_notCompilation = true;
            break;
        case CompilationUnknown:
            m_isCompilation = m_notCompilation = false;
            break;
    }
}

void MetaBundle::setLength( int length )
{ aboutToChange( Length ); m_length = length; reactToChange( Length ); }

void MetaBundle::setBitrate( int bitrate )
{ aboutToChange( Bitrate ); m_bitrate = bitrate; reactToChange( Bitrate ); }

void MetaBundle::setSampleRate( int sampleRate )
{ aboutToChange( SampleRate ); m_sampleRate = sampleRate; reactToChange( SampleRate ); }

void MetaBundle::setDiscNumber( int discnumber )
{ aboutToChange( DiscNumber ); m_discNumber = discnumber; reactToChange( DiscNumber ); }

void MetaBundle::setComposer( const AtomicString &composer )
{ aboutToChange( Composer ); m_composer = composer; reactToChange( Composer ); }

void MetaBundle::setPlayCount( int playcount )
{ aboutToChange( PlayCount ); m_playCount = playcount; reactToChange( PlayCount ); }

void MetaBundle::setLastPlay( uint lastplay )
{ aboutToChange( LastPlayed ); m_lastPlay = lastplay; reactToChange( LastPlayed ); }

void MetaBundle::setRating( int rating )
{ aboutToChange( Rating ); m_rating = rating; reactToChange( Rating ); }

void MetaBundle::setScore( int score )
{ aboutToChange( Score ); m_score = score; reactToChange( Score ); }

void MetaBundle::setFilesize( int bytes )
{ aboutToChange( Filesize ); m_filesize = bytes; reactToChange( Filesize ); }

void MetaBundle::setFileType( int type ) { m_type = type; }

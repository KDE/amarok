// Max Howell <max.howell@methylblue.com>, (C) 2004
// Alexandre Pereira de Oliveira <aleprj@gmail.com>, (C) 2005, 2006
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
#include <sys/types.h>
#include <fcntl.h>

#include "amarok.h"
#include "amarokconfig.h"
#include "debug.h"
#include "collectiondb.h"
#include "metabundlesaver.h"
#include <kapplication.h>
#include <kfilemetainfo.h>
#include <kio/global.h>
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kio/netaccess.h>
#include <kmdcodec.h>
#include <qdeepcopy.h>
#include <qfile.h> //decodePath()
#include <taglib/attachedpictureframe.h>
#include <taglib/fileref.h>
#include <taglib/id3v1genres.h> //used to load genre list
#include <taglib/mpegfile.h>
#include <taglib/tag.h>
#include <taglib/tstring.h>
#include <taglib/tlist.h>
#include <taglib/apetag.h>
#include <taglib/id3v2tag.h>
#include <taglib/id3v1tag.h>
#include <taglib/mpcfile.h>
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


namespace Amarok {
    KURL detachedKURL( const KURL &url ) {
        KURL urlCopy;
        if (!url.isEmpty())
            urlCopy = KURL(url.url());
        return urlCopy;
    }
}


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
const QString &MetaBundle::exactColumnName( int c ) //static
{
    // construct static qstrings to avoid constructing them all the time
    static QString columns[] = {
        "Filename", "Title", "Artist", "AlbumArtist", "Composer", "Year", "Album", "DiscNumber", "Track", "BPM", "Genre", "Comment",
        "Directory", "Type", "Length", "Bitrate", "SampleRate", "Score", "Rating", "PlayCount", "LastPlayed",
        "Mood", "Filesize" };
    static QString error( "ERROR" );

    if ( c >= 0 && c < NUM_COLUMNS )
        return columns[c];
    else
        return error;
}

const QString MetaBundle::prettyColumnName( int index ) //static
{
    switch( index )
    {
        case Filename:   return i18n( "Filename"    );
        case Title:      return i18n( "Title"       );
        case Artist:     return i18n( "Artist"      );
        case AlbumArtist:return i18n( "Album Artist");
        case Composer:   return i18n( "Composer"    );
        case Year:       return i18n( "Year"        );
        case Album:      return i18n( "Album"       );
        case DiscNumber: return i18n( "Disc Number" );
        case Track:      return i18n( "Track"       );
        case Bpm:        return i18n( "BPM"         );
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
        case LastPlayed: return i18n( "Column name", "Last Played" );
        case Mood:       return i18n( "Mood"        );
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
        : m_uniqueId( QString::null )
        , m_year( Undetermined )
        , m_discNumber( Undetermined )
        , m_track( Undetermined )
        , m_bpm( Undetermined )
        , m_bitrate( Undetermined )
        , m_length( Undetermined )
        , m_sampleRate( Undetermined )
        , m_score( Undetermined )
        , m_rating( Undetermined )
        , m_playCount( Undetermined )
        , m_lastPlay( abs( Undetermined ) )
        , m_filesize( Undetermined )
        , m_moodbar( 0 )
        , m_type( other )
        , m_exists( true )
        , m_isValidMedia( true )
        , m_isCompilation( false )
        , m_notCompilation( false )
        , m_safeToSave( false )
        , m_waitingOnKIO( 0 )
        , m_tempSavePath( QString::null )
        , m_origRenamedSavePath( QString::null )
        , m_tempSaveDigest( 0 )
        , m_saveFileref( 0 )
        , m_podcastBundle( 0 )
        , m_lastFmBundle( 0 )
        , m_isSearchDirty(true)
        , m_searchColumns( Undetermined )
{
    init();
}

MetaBundle::MetaBundle( const KURL &url, bool noCache, TagLib::AudioProperties::ReadStyle readStyle, EmbeddedImageList* images )
    : m_url( url )
    , m_uniqueId( QString::null )
    , m_year( Undetermined )
    , m_discNumber( Undetermined )
    , m_track( Undetermined )
    , m_bpm( Undetermined )
    , m_bitrate( Undetermined )
    , m_length( Undetermined )
    , m_sampleRate( Undetermined )
    , m_score( Undetermined )
    , m_rating( Undetermined )
    , m_playCount( Undetermined )
    , m_lastPlay( abs( Undetermined ) )
    , m_filesize( Undetermined )
    , m_moodbar( 0 )
    , m_type( other )
    , m_exists( isFile() && QFile::exists( url.path() ) )
    , m_isValidMedia( false )
    , m_isCompilation( false )
    , m_notCompilation( false )
    , m_safeToSave( false )
    , m_waitingOnKIO( 0 )
    , m_tempSavePath( QString::null )
    , m_origRenamedSavePath( QString::null )
    , m_tempSaveDigest( 0 )
    , m_saveFileref( 0 )
    , m_podcastBundle( 0 )
    , m_lastFmBundle( 0 )
    , m_isSearchDirty(true)
    , m_searchColumns( Undetermined )
{
    if ( exists() )
    {
        if ( !noCache )
            m_isValidMedia = CollectionDB::instance()->bundleForUrl( this );

        if ( !isValidMedia() || ( !m_podcastBundle && m_length <= 0 ) )
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
        , m_uniqueId( QString::null )
        , m_year( 0 )
        , m_discNumber( 0 )
        , m_track( 0 )
        , m_bpm( Undetermined )
        , m_bitrate( bitrate )
        , m_length( Irrelevant )
        , m_sampleRate( Unavailable )
        , m_score( Undetermined )
        , m_rating( Undetermined )
        , m_playCount( Undetermined )
        , m_lastPlay( abs( Undetermined ) )
        , m_filesize( Undetermined )
        , m_moodbar( 0 )
        , m_type( other )
        , m_exists( true )
        , m_isValidMedia( false )
        , m_isCompilation( false )
        , m_notCompilation( false )
        , m_safeToSave( false )
        , m_waitingOnKIO( 0 )
        , m_tempSavePath( QString::null )
        , m_origRenamedSavePath( QString::null )
        , m_tempSaveDigest( 0 )
        , m_saveFileref( 0 )
        , m_podcastBundle( 0 )
        , m_lastFmBundle( 0 )
        , m_isSearchDirty( true )
        , m_searchColumns( Undetermined )
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
        : m_moodbar( 0 )
{
    *this = bundle;
}

MetaBundle::~MetaBundle()
{
    delete m_podcastBundle;
    delete m_lastFmBundle;

    if( m_moodbar != 0 )
      delete m_moodbar;
}

MetaBundle&
MetaBundle::operator=( const MetaBundle& bundle )
{
    m_url = bundle.m_url;
    m_title = bundle.m_title;
    m_artist = bundle.m_artist;
    m_albumArtist = bundle.m_albumArtist;
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
    m_bpm = bundle.m_bpm;
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
    m_safeToSave = bundle.m_safeToSave;
    m_waitingOnKIO = bundle.m_waitingOnKIO;
    m_tempSavePath = bundle.m_tempSavePath;
    m_origRenamedSavePath = bundle.m_origRenamedSavePath;
    m_tempSaveDigest = bundle.m_tempSaveDigest;
    m_saveFileref = bundle.m_saveFileref;

    if( bundle.m_moodbar != 0)
      {
        if( m_moodbar == 0 )
          m_moodbar = new Moodbar( this );
        *m_moodbar = *bundle.m_moodbar;
      }
    else
      {
        // If m_moodbar != 0, it's initialized for a reason
        // Deleting it makes the PrettySlider code more ugly,
        // since it'd have to reconnect the jobEvent() signal.
        if( m_moodbar != 0 )
          m_moodbar->reset();
      }


//    delete m_podcastBundle; why does this crash Amarok? apparently m_podcastBundle isn't always initialized.
    m_podcastBundle = 0;
    if( bundle.m_podcastBundle )
        setPodcastBundle( *bundle.m_podcastBundle );

//    delete m_lastFmBundle; same as above
    m_lastFmBundle = 0;
    if( bundle.m_lastFmBundle )
        setLastFmBundle( *bundle.m_lastFmBundle );

	m_isSearchDirty = true;
    return *this;
}


bool
MetaBundle::checkExists()
{
    m_exists = !isFile() || QFile::exists( url().path() );

    return m_exists;
}

bool
MetaBundle::operator==( const MetaBundle& bundle ) const
{
    return uniqueId()   == bundle.uniqueId() && //first, since if using IDs will return faster
           artist()     == bundle.artist() &&
           albumArtist() == bundle.albumArtist() &&
           title()      == bundle.title() &&
           composer()   == bundle.composer() &&
           album()      == bundle.album() &&
           year()       == bundle.year() &&
           comment()    == bundle.comment() &&
           genre()      == bundle.genre() &&
           track()      == bundle.track() &&
           discNumber() == bundle.discNumber() &&
           bpm()        == bundle.bpm() &&
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
MetaBundle::embeddedImages( MetaBundle::EmbeddedImageList& images ) const
{
    if ( isFile() )
    {
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
    if( !isFile() )
        return;

    const QString path = url().path();

    TagLib::FileRef fileref;
    TagLib::Tag *tag = 0;
    fileref = TagLib::FileRef( QFile::encodeName( path ), true, readStyle );

    if( !fileref.isNull() )
    {
        setUniqueId( readUniqueId( &fileref ) );
        m_filesize = QFile( path ).size();

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
        }


    /* As mpeg implementation on TagLib uses a Tag class that's not defined on the headers,
       we have to cast the files, not the tags! */

        QString disc;
        QString compilation;
        if ( TagLib::MPEG::File *file = dynamic_cast<TagLib::MPEG::File *>( fileref.file() ) )
        {
            m_type = mp3;
            if ( file->ID3v2Tag() )
            {
                if ( !file->ID3v2Tag()->frameListMap()["TPOS"].isEmpty() )
                    disc = TStringToQString( file->ID3v2Tag()->frameListMap()["TPOS"].front()->toString() ).stripWhiteSpace();

                if ( !file->ID3v2Tag()->frameListMap()["TBPM"].isEmpty() )
                    setBpm( TStringToQString( file->ID3v2Tag()->frameListMap()["TBPM"].front()->toString() ).stripWhiteSpace().toFloat() );

                if ( !file->ID3v2Tag()->frameListMap()["TCOM"].isEmpty() )
                    setComposer( TStringToQString( file->ID3v2Tag()->frameListMap()["TCOM"].front()->toString() ).stripWhiteSpace() );

                if ( !file->ID3v2Tag()->frameListMap()["TPE2"].isEmpty() ) // non-standard: Apple, Microsoft
                    setAlbumArtist( TStringToQString( file->ID3v2Tag()->frameListMap()["TPE2"].front()->toString() ).stripWhiteSpace() );

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

                if ( !file->tag()->fieldListMap()[ "BPM" ].isEmpty() )
                    setBpm( TStringToQString( file->tag()->fieldListMap()["BPM"].front() ).stripWhiteSpace().toFloat() );

                if ( !file->tag()->fieldListMap()[ "DISCNUMBER" ].isEmpty() )
                    disc = TStringToQString( file->tag()->fieldListMap()["DISCNUMBER"].front() ).stripWhiteSpace();

                if ( !file->tag()->fieldListMap()[ "COMPILATION" ].isEmpty() )
                    compilation = TStringToQString( file->tag()->fieldListMap()["COMPILATION"].front() ).stripWhiteSpace();
            }
        }
        else if ( TagLib::FLAC::File *file = dynamic_cast<TagLib::FLAC::File *>( fileref.file() ) )
        {
            m_type = flac;
            if ( file->xiphComment() )
            {
                if ( !file->xiphComment()->fieldListMap()[ "COMPOSER" ].isEmpty() )
                    setComposer( TStringToQString( file->xiphComment()->fieldListMap()["COMPOSER"].front() ).stripWhiteSpace() );

                if ( !file->xiphComment()->fieldListMap()[ "BPM" ].isEmpty() )
                    setBpm( TStringToQString( file->xiphComment()->fieldListMap()["BPM"].front() ).stripWhiteSpace().toFloat() );

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
                setBpm( QString::number( mp4tag->bpm() ).toFloat() );
                disc = QString::number( mp4tag->disk() );
                compilation = QString::number( mp4tag->compilation() );
                if ( images && mp4tag->cover().size() ) {
                    images->push_back( EmbeddedImage( mp4tag->cover(), "" ) );
                }
            }
        }

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
    if( !isFile() )
    {
        m_filesize = Undetermined;
        return;
    }

    const QString path = url().path();
    m_filesize = QFile( path ).size();
}

float MetaBundle::score( bool ensureCached ) const
{
    if( m_score == Undetermined && !ensureCached )
        //const_cast is ugly, but other option was mutable, and then we lose const correctness checking
        //everywhere else
        *const_cast<float*>(&m_score) = CollectionDB::instance()->getSongPercentage( m_url.path() );
    return m_score;
}

int MetaBundle::rating( bool ensureCached ) const
{
    if( m_rating == Undetermined && !ensureCached )
        *const_cast<int*>(&m_rating) = CollectionDB::instance()->getSongRating( m_url.path() );
    return m_rating;
}

int MetaBundle::playCount( bool ensureCached ) const
{
    if( m_playCount == Undetermined && !ensureCached )
        *const_cast<int*>(&m_playCount) = CollectionDB::instance()->getPlayCount( m_url.path() );
    return m_playCount;
}

uint MetaBundle::lastPlay( bool ensureCached ) const
{
    if( (int)m_lastPlay == abs(Undetermined) && !ensureCached )
        *const_cast<uint*>(&m_lastPlay) = CollectionDB::instance()->getLastPlay( m_url.path() ).toTime_t();
    return m_lastPlay;
}

void MetaBundle::copyFrom( const MetaBundle &bundle )
{
    setTitle( bundle.title() );
    setArtist( bundle.artist() );
    setAlbumArtist( bundle.albumArtist() );
    setComposer( bundle.composer() );
    setAlbum( bundle.album() );
    setYear( bundle.year() );
    setDiscNumber( bundle.discNumber() );
    setBpm( bundle.bpm() );
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

void MetaBundle::copyFrom( const PodcastEpisodeBundle &peb )
{
    setPodcastBundle( peb );
    setTitle( peb.title() );
    setArtist( peb.author() );
    PodcastChannelBundle pcb;
    if( CollectionDB::instance()->getPodcastChannelBundle( peb.parent(), &pcb ) )
    {
        if( !pcb.title().isEmpty() )
            setAlbum( pcb.title() );
    }
    setGenre( QString ( "Podcast" ) );
}

void MetaBundle::setExactText( int column, const QString &newText )
{
    switch( column )
    {
        case Title:      setTitle(      newText );           break;
        case Artist:     setArtist(     newText );           break;
        case AlbumArtist: setAlbumArtist( newText );         break;
        case Composer:   setComposer(   newText );           break;
        case Year:       setYear(       newText.toInt() );   break;
        case Album:      setAlbum(      newText );           break;
        case DiscNumber: setDiscNumber( newText.toInt() );   break;
        case Track:      setTrack(      newText.toInt() );   break;
        case Bpm:        setBpm(        newText.toFloat() ); break;
        case Genre:      setGenre(      newText );           break;
        case Comment:    setComment(    newText );           break;
        case Length:     setLength(     newText.toInt() );   break;
        case Bitrate:    setBitrate(    newText.toInt() );   break;
        case SampleRate: setSampleRate( newText.toInt() );   break;
        case Score:      setScore(      newText.toFloat() ); break;
        case Rating:     setRating(     newText.toInt() );   break;
        case PlayCount:  setPlayCount(  newText.toInt() );   break;
        case LastPlayed: setLastPlay(   newText.toInt() );   break;
        case Filesize:   setFilesize(   newText.toInt() );   break;
        case Type:       setFileType(   newText.toInt() );   break;
        default: warning() << "Tried to set the text of an immutable or nonexistent column! [" << column << endl;
   }
}

QString MetaBundle::exactText( int column, bool ensureCached ) const
{
    switch( column )
    {
        case Filename:   return filename();
        case Title:      return title();
        case Artist:     return artist();
        case AlbumArtist: return albumArtist();
        case Composer:   return composer();
        case Year:       return QString::number( year() );
        case Album:      return album();
        case DiscNumber: return QString::number( discNumber() );
        case Track:      return QString::number( track() );
        case Bpm:        return QString::number( bpm() );
        case Genre:      return genre();
        case Comment:    return comment();
        case Directory:  return directory();
        case Type:       return QString::number( fileType() );
        case Length:     return QString::number( length() );
        case Bitrate:    return QString::number( bitrate() );
        case SampleRate: return QString::number( sampleRate() );
        case Score:      return QString::number( score( ensureCached ) );
        case Rating:     return QString::number( rating( ensureCached ) );
        case PlayCount:  return QString::number( playCount( ensureCached ) );
        case LastPlayed: return QString::number( lastPlay( ensureCached ) );
        case Filesize:   return QString::number( filesize() );
        case Mood:       return QString();
        default: warning() << "Tried to get the text of a nonexistent column! [" << column << endl;
    }

    return QString(); //shouldn't happen
}

QString MetaBundle::prettyText( int column ) const
{
    QString text;
    switch( column )
    {
        case Filename:   text = isFile() ? MetaBundle::prettyTitle(filename()) : url().prettyURL();  break;
        case Title:      text = title().isEmpty() ? MetaBundle::prettyTitle( filename() ) : title(); break;
        case Artist:     text = artist();                                                            break;
        case AlbumArtist: text = albumArtist();                                                      break;
        case Composer:   text = composer();                                                          break;
        case Year:       text = year() ? QString::number( year() ) : QString::null;                  break;
        case Album:      text = album();                                                             break;
        case DiscNumber: text = discNumber() ? QString::number( discNumber() ) : QString::null;      break;
        case Bpm:        text = bpm() ? QString::number( bpm() ) : QString::null;                    break;
        case Track:      text = track() ? QString::number( track() ) : QString::null;                break;
        case Genre:      text = genre();                                                             break;
        case Comment:    text = comment();                                                           break;
        case Directory:  text = url().isEmpty() ? QString() : directory();                           break;
        case Type:       text = url().isEmpty() ? QString() : type();                                break;
        case Length:     text = prettyLength( length(), true );                                      break;
        case Bitrate:    text = prettyBitrate( bitrate() );                                          break;
        case SampleRate: text = prettySampleRate();                                                  break;
        case Score:      text = QString::number( static_cast<int>( score() ) );                      break;
        case Rating:     text = prettyRating();                                                      break;
        case PlayCount:  text = QString::number( playCount() );                                      break;
        case LastPlayed: text = Amarok::verboseTimeSince( lastPlay() );                              break;
        case Filesize:   text = prettyFilesize();                                                    break;
        case Mood:
          text = moodbar_const().state() == Moodbar::JobRunning ? i18n( "Calculating..." )
               : moodbar_const().state() == Moodbar::JobQueued  ? i18n( "Queued..." )
               : QString::null;
        break;
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

void MetaBundle::reactToChanges( const QValueList<int>& columns)
{
    // mark search dirty if we need to
    for (uint i = 0; !m_isSearchDirty && i < columns.count(); i++)
        if ((m_searchColumns & (1 << columns[i])) > 0)
            m_isSearchDirty = true;
}

bool MetaBundle::matchesFast(const QStringList &terms, ColumnMask columnMask) const
{
    // simple search for rating, last played, etc. makes no sense and it hurts us a
    // lot if we have to fetch it from the db. so zero them out
    columnMask &= ~( 1<<Score | 1<<Rating | 1<<PlayCount | 1<<LastPlayed | 1<<Mood );

    if (m_isSearchDirty || m_searchColumns != columnMask) {
        // assert the size of ColumnMask is large enough. In the absence of
        // a compile assert mechanism, this is pretty much as good for
        // optimized code (ie, free)

        if ( sizeof(ColumnMask) < (NUM_COLUMNS / 8) ) {
            warning() << "ColumnMask is not big enough!\n";
        }

        // recompute search text
        // There is potential for mishap here if matchesFast gets called from multiple
        // threads, but it's *highly* unlikely that something bad will happen
        m_isSearchDirty = false;
        m_searchColumns = columnMask;
        m_searchStr.setLength(0);

        for (int i = 0; i < NUM_COLUMNS; i++) {
            if ((columnMask & (1 << i)) > 0) {
                if (!m_searchStr.isEmpty()) m_searchStr += ' ';
                m_searchStr += prettyText(i).lower();
            }
        }
    }

    // now search
    for (uint i = 0; i < terms.count(); i++) {
        if (!m_searchStr.contains(terms[i])) return false;
    }

    return true;
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
            {
                QString field = e.field.lower();
                column = columnIndex( field );
                if( column == -1 )
                {
                    column = -2;
                    if( field == "size" )
                        field = "filesize";
                    else if( field == "filetype" )
                        field = "type";
                    else if( field == "disc" )
                        field = "discnumber";
                    else
                        column = -1;

                    if( column == -2 )
                        column = columnIndex( field );
                }
            }
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
                    case Bpm:
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

    return QString(); //Unavailable = ""
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
    min = time % 60; //minutes
    time /= 60;
    hr = time % 24 ; //hours
    time /= 24;
    day = time % 7 ; //days
    time /= 7;
    week = time; //weeks

    if( week && hr >= 12 )
    {
        day++;
        if( day == 7 )
        {
            week++;
            day = 0;
        }
    }
    else if( day && min >= 30 )
    {
        hr++;
        if( hr == 24 )
        {
            day++;
            hr = 0;
        }
    }
    else if( hr && secs >= 30 )
    {
        min++;
        if( min == 60 )
        {
            hr++;
            min = 0;
        }
    }

    QString weeks = i18n( "1 week %1", "%n weeks %1", week );
    QString days = i18n( "1 day %1", "%n days %1", day );
    QString hours = i18n( "1 hour", "%n hours", hr );

    if( week )
        return weeks.arg( day ? days.arg("") : "" ).simplifyWhiteSpace();
    else if ( day )
        return days.arg( hr ? hours : "" ).simplifyWhiteSpace();
    else if ( hr )
        return i18n( "%1:%2 hours" ).arg( hr ).arg( zeroPad( min ) );
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
        case 1: return i18n( "Awful" );
        case 2: return i18n( "Bad" );
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
    for ( int i = 1; i<=10; i++ )
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
    const char *id = 0;

    if ( m_type == mp3 )
    {
        switch( tag )
        {
            case ( composerTag ): id = "TCOM"; break;
            case ( discNumberTag ): id = "TPOS"; break;
            case ( bpmTag ): id = "TBPM"; break;
            case ( compilationTag ): id = "TCMP"; break;
            case ( albumArtistTag ): id = "TPE2"; break; // non-standard: Apple, Microsoft
        }
        fprintf(stderr, "Setting extended tag %s to %s\n", id, value.utf8().data());
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
            case ( bpmTag ): id = "BPM"; break;
            case ( compilationTag ): id = "COMPILATION"; break;
            case ( albumArtistTag ): id = "ALBUMARTIST"; break; // non-standard: Amarok
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
            case ( bpmTag ): id = "BPM"; break;
            case ( compilationTag ): id = "COMPILATION"; break;
            case ( albumArtistTag ): id = "ALBUMARTIST"; break; // non-standard: Amarok
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
                case ( bpmTag ): mp4tag->setBpm( value.toInt() ); // mp4 doesn't support float bpm
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

void MetaBundle::loadImagesFromTag( const TagLib::ID3v2::Tag &tag, EmbeddedImageList& images ) const
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
MetaBundle::safeSave()
{
    bool noproblem;
    MetaBundleSaver mbs( this );
    TagLib::FileRef* fileref = mbs.prepareToSave();
    if( !fileref )
    {
        debug() << "Could not get a fileref!" << endl;
        mbs.cleanupSave();
        return false;
    }

    noproblem = save( fileref );

    if( !noproblem )
    {
        debug() << "MetaBundle::save() didn't work!" << endl;
        mbs.cleanupSave();
        return false;
    }

    noproblem = mbs.doSave();

    if( !noproblem )
    {
        debug() << "Something failed during the save, cleaning up and exiting!" << endl;
        mbs.cleanupSave();
        return false;
    }

    setUniqueId( readUniqueId() );
    if( CollectionDB::instance()->isFileInCollection( url().path() ) )
        CollectionDB::instance()->doAFTStuff( this, false );

    noproblem = mbs.cleanupSave();

    return noproblem;
}

bool
MetaBundle::save( TagLib::FileRef* fileref )
{
    DEBUG_BLOCK
    if( !isFile() )
        return false;

    //Set default codec to UTF-8 (see bugs 111246 and 111232)
    TagLib::ID3v2::FrameFactory::instance()->setDefaultTextEncoding(TagLib::String::UTF8);

    bool passedin = fileref;
    bool returnval = false;

    TagLib::FileRef* f;

    if( !passedin )
        f = new TagLib::FileRef( QFile::encodeName( url().path() ), false );
    else
        f = fileref;

    if ( f && !f->isNull() )
    {
        TagLib::Tag * t = f->tag();
        if ( t ) { // f.tag() can return null if the file couldn't be opened for writing
            t->setTitle( QStringToTString( title().stripWhiteSpace() ) );
            t->setArtist( QStringToTString( artist().string().stripWhiteSpace() ) );
            t->setAlbum( QStringToTString( album().string().stripWhiteSpace() ) );
            t->setTrack( track() );
            t->setYear( year() );
            t->setComment( QStringToTString( comment().string().stripWhiteSpace() ) );
            t->setGenre( QStringToTString( genre().string().stripWhiteSpace() ) );

            if ( hasExtendedMetaInformation() )
            {
                setExtendedTag( f->file(), albumArtistTag, albumArtist() );
                setExtendedTag( f->file(), composerTag, composer().string().stripWhiteSpace() );
                setExtendedTag( f->file(), discNumberTag, discNumber() ? QString::number( discNumber() ) : QString() );
                setExtendedTag( f->file(), bpmTag, bpm() ? QString::number( bpm() ) : QString() );
                if ( compilation() != CompilationUnknown )
                    setExtendedTag( f->file(), compilationTag, QString::number( compilation() ) );
            }
            if( !passedin )
            {
                returnval = f->save();
                setUniqueId( readUniqueId() );
                if( returnval && CollectionDB::instance()->isFileInCollection( url().path() ) )
                    CollectionDB::instance()->doAFTStuff( this, false );
            }
            else
                returnval = true;
        }
    }
    if ( !passedin )
        delete f;

    return returnval;
}

// QT's version encodeAttr is 1) private to QDom, and 2) a litte slow. This
// one can be made public if needed. It happens to be on a critical path
// (each char of playlist / undo save). QStyleSheet::escape does not deal with
// unicode chars illegal for XML. There's a lot of junk in those tags
static inline void xmlEncode(QTextStream &stream, const QString &str)
{
    QString tmp;
    const QString *cur = &str;
    uint i = 0;
    while ( i < cur->length() )
    {
        uint uc = (*cur)[i].unicode();
        // we try to accumulate unescaped chars before writing to stream
        const char *escaped;
        // careful about the order of tests, common before less common
        if ( 'a' <= uc && uc <= 'z' || '0' <= uc && uc <= '9' || 'A' <= uc && uc <= 'Z' )
            // most common case
            escaped = NULL;
        else if ( uc == '<' ) escaped = "&lt;";
        else if ( uc == '>' ) escaped = "&gt;";
        else if ( uc == '&' ) escaped = "&amp;";
        else if ( uc == '"' ) escaped = "&quot;";
        else
        {
            // see if it's a XML-valid unicode char at all
            if ( (0x20 <= uc && uc <= 0xD7FF || 0xE000 <= uc && uc <= 0xFFFD
                  || uc == 0x9 || uc == 0xA || uc == 0xD) )
                // fairly common, other ascii chars
                escaped = NULL;
            else
                escaped = "";   // special char, will write later
        }
        if ( escaped )
        {
            // flush previous unescaped chars
            if ( i > 0 ) stream << cur->left(i);
            tmp = cur->right(cur->length() - i - 1);
            cur = &tmp;
            i = 0;
            // now write escaped string
            if ( escaped[0] ) stream << escaped;
            else stream << "&#x" << QString::number(uc, 16) << ';';
        } else i++;
    }

    if (!cur->isEmpty()) stream << *cur;
}

bool MetaBundle::save(QTextStream &stream, const QStringList &attributes) const
{
    // QDom is too slow to use here
    stream << " <item url=\"";
    xmlEncode( stream, url().url() );
    stream << "\" uniqueid=\"" << uniqueId() << '\"';
    if ( m_isCompilation )
        stream << " compilation=\"True\"";
    // attrs are never custom text (e.g. ID3 tags)
    for( int i = 0, n = attributes.count(); i < n; i += 2 )
        stream << " " << attributes[i] << "=\"" << attributes[i+1] << "\"";
    stream << ">\n";            // end of <item ...>
    for (int i = 0; i < NUM_COLUMNS; ++i) {
        if ( i == Filename ) continue;  // the file name is already in the URL
        const QString &tag = exactColumnName( i );
        // 2 indents
        stream << "  <" << tag << ">";
        xmlEncode( stream, exactText( i, true ) );
        stream << "</" << tag << ">\n";
    }
    stream << " </item>\n";
    return true;
}

void MetaBundle::setUrl( const KURL &url )
{
    QValueList<int> changes;
    for( int i = 0; i < NUM_COLUMNS; ++i ) changes << i;
    aboutToChange( changes ); m_url = url; reactToChanges( changes );

    setUniqueId();
}

void MetaBundle::setPath( const QString &path )
{
    QValueList<int> changes;
    for( int i = 0; i < NUM_COLUMNS; ++i ) changes << i;
    aboutToChange( changes ); m_url.setPath( path ); reactToChanges( changes );

    setUniqueId();
}

void MetaBundle::setUniqueId()
{
    if( !isFile() )
        return;

    m_uniqueId = CollectionDB::instance()->uniqueIdFromUrl( url() );
}

void MetaBundle::setUniqueId( const QString &id )
{
    //WARNING WARNING WARNING
    //Don't call this function if you don't know what you're doing!
    m_uniqueId = id;
}

const TagLib::ByteVector
MetaBundle::readUniqueIdHelper( TagLib::FileRef fileref ) const
{
    if ( TagLib::MPEG::File *file = dynamic_cast<TagLib::MPEG::File *>( fileref.file() ) )
    {
        if( file->ID3v2Tag() )
            return file->ID3v2Tag()->render();
        else if( file->ID3v1Tag() )
            return file->ID3v1Tag()->render();
        else if( file->APETag() )
            return file->APETag()->render();
    }
    else if ( TagLib::Ogg::Vorbis::File *file = dynamic_cast<TagLib::Ogg::Vorbis::File *>( fileref.file() ) )
    {
        if( file->tag() )
            return file->tag()->render();
    }
    else if ( TagLib::FLAC::File *file = dynamic_cast<TagLib::FLAC::File *>( fileref.file() ) )
    {
        if( file->ID3v2Tag() )
            return file->ID3v2Tag()->render();
        else if( file->ID3v1Tag() )
            return file->ID3v1Tag()->render();
        else if( file->xiphComment() )
            return file->xiphComment()->render();
    }
    else if ( TagLib::Ogg::FLAC::File *file = dynamic_cast<TagLib::Ogg::FLAC::File *>( fileref.file() ) )
    {
        if( file->tag() )
            return file->tag()->render();
    }
    else if ( TagLib::MPC::File *file = dynamic_cast<TagLib::MPC::File *>( fileref.file() ) )
    {
        if( file->ID3v1Tag() )
            return file->ID3v1Tag()->render();
        else if( file->APETag() )
            return file->APETag()->render();
    }
    TagLib::ByteVector bv;
    return bv;
}

QString
MetaBundle::readUniqueId( TagLib::FileRef* fileref )
{
    //This is used in case we don't get given a fileref
    TagLib::FileRef tmpfileref;

    if( !fileref && isFile() )
    {
        const QString path = url().path();
        //Make it get cleaned up at the end of the function automagically
        tmpfileref = TagLib::FileRef( QFile::encodeName( path ), true, TagLib::AudioProperties::Fast );
        fileref = &tmpfileref;
    }

    if( !fileref || fileref->isNull() )
        return QString();

    TagLib::ByteVector bv = readUniqueIdHelper( *fileref );

    //get our unique id
    KMD5 md5( 0, 0 );

    QFile qfile( url().path() );

    char databuf[8192];
    int readlen = 0;
    QCString size = 0;
    QString returnval;

    md5.update( bv.data(), bv.size() );

    if( qfile.open( IO_Raw | IO_ReadOnly ) )
    {
        if( ( readlen = qfile.readBlock( databuf, 8192 ) ) > 0 )
        {
            md5.update( databuf, readlen );
            md5.update( size.setNum( (ulong)qfile.size() ) );
            return QString( md5.hexDigest().data() );
        }
        else
            return QString();
    }

    return QString::null;
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
MetaBundle::getRandomString( int size, bool numbersOnly )
{
    if( size != 8 )
    {
        debug() << "Wrong size passed in!" << endl;
        return QString();
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
        int r=rand() % 94;
        // shift the value to the visible characters
        r+=33;
        // we don't want ", %, ', <, >, \, `, or &
        // so that we don't have issues with escaping/quoting in QStrings,
        // and so that we don't have <> in our XML files where they might cause issues
        // hopefully this list is final, as once users really start using this
        // it will be a pain to change...however, there is an ATF version in CollectionDB
        // which will help if this ever needs to change
        // In addition we can change our vendor string
        while ( r==34 || r==37 || r == 38 || r==39 || r==60 ||r == 62 || r==92 || r==96 )
            r++;

        if( numbersOnly && ( r < 48 || r > 57 ) )
        {
            size++;
            continue;
        }

        str[i++] =  char(r);
        // this next comment kept in for fun, as it was from the source of KRandomString, where I got
        // most of this code from to start with :-)
        // so what if I work backwards?
    }
    return str;
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

void MetaBundle::setBpm( float bpm )
{ aboutToChange( Bpm ); m_bpm = bpm; reactToChange( Bpm ); }

void MetaBundle::setComposer( const AtomicString &composer )
{ aboutToChange( Composer ); m_composer = composer; reactToChange( Composer ); }

void MetaBundle::setAlbumArtist( const AtomicString &albumArtist )
{ aboutToChange( AlbumArtist ); m_albumArtist = albumArtist; reactToChange( AlbumArtist ); }

void MetaBundle::setPlayCount( int playcount )
{ aboutToChange( PlayCount ); m_playCount = playcount; reactToChange( PlayCount ); }

void MetaBundle::setLastPlay( uint lastplay )
{ aboutToChange( LastPlayed ); m_lastPlay = lastplay; reactToChange( LastPlayed ); }

void MetaBundle::setRating( int rating )
{ aboutToChange( Rating ); m_rating = rating; reactToChange( Rating ); }

void MetaBundle::setScore( float score )
{ aboutToChange( Score ); m_score = score; reactToChange( Score ); }

void MetaBundle::setFilesize( int bytes )
{ aboutToChange( Filesize ); m_filesize = bytes; reactToChange( Filesize ); }

void MetaBundle::setFileType( int type ) { m_type = type; }

void MetaBundle::detach()
{
    // FIXME: we'd do that, but unfortunately it does not exist
    //m_url.detach();
    m_url = Amarok::detachedKURL( m_url );

    m_title = QDeepCopy<QString>(m_title);
    m_artist = m_artist.deepCopy();
    m_albumArtist = m_albumArtist.deepCopy();
    m_album = m_album.deepCopy();
    m_comment = m_comment.deepCopy();
    m_composer = m_composer.deepCopy();
    m_genre = m_genre.deepCopy();
    m_streamName = QDeepCopy<QString>(m_streamName);
    m_streamUrl = QDeepCopy<QString>(m_streamUrl);

    if( m_moodbar != 0 )
      m_moodbar->detach();

    m_uniqueId = QDeepCopy<QString>( m_uniqueId );

    if ( m_podcastBundle )
         setPodcastBundle( QDeepCopy<PodcastEpisodeBundle>( *m_podcastBundle ) );
    if ( m_lastFmBundle )
         setLastFmBundle( QDeepCopy<LastFm::Bundle>( *m_lastFmBundle ) );
}


void PodcastEpisodeBundle::detach()
{
    m_url = Amarok::detachedKURL( m_url );
    m_localUrl = Amarok::detachedKURL( m_localUrl );
    m_parent = Amarok::detachedKURL( m_parent );

    m_author = QDeepCopy<QString>(m_author);
    m_title = QDeepCopy<QString>(m_title);
    m_subtitle = QDeepCopy<QString>(m_subtitle);
    m_description = QDeepCopy<QString>(m_subtitle);
    m_date =  QDeepCopy<QString>(m_date);
    m_type = QDeepCopy<QString>(m_type);
    m_guid = QDeepCopy<QString>(m_guid);
}

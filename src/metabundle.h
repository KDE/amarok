// Max Howell <max.howell@methylblue.com>, (C) 2004
// Alexandre Pereira de Oliveira <aleprj@gmail.com>, (C) 2005
// Shane King <kde@dontletsstart.com>, (C) 2006
// Peter C. Ndikuwera <pndiku@gmail.com>, (C) 2006
// License: GNU General Public License V2

#ifndef METABUNDLE_H
#define METABUNDLE_H

#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
#define PRETTY_TITLE_CACHE
#endif

#include <qstringlist.h>
#include <kurl.h>    //inline functions
#include <klocale.h> //inline functions
#include <taglib/audioproperties.h>
#include "expression.h"
#include "atomicstring.h"
#include "moodbar.h"

#include "amarok_export.h"

class KFileMetaInfo;
class QDir;
class QTextStream;
template<class T> class QValueList;
namespace TagLib {
    class ByteVector;
    class File;
    class FileRef;
    class String;
    namespace ID3v2 {
        class UniqueFileIdentifierFrame;
        class Tag;
    }
    namespace MPEG {
        class File;
    }
}
class PodcastEpisodeBundle;

namespace LastFm {
    class Bundle;
}

/**
 * @class MetaBundle
 * @author Max Howell <max.howell@methylblue.com>
 *
 * If this class doesn't work for you in some way, extend it sensibly :)
 *
 */

class LIBAMAROK_EXPORT MetaBundle
{

public:
    enum Column
    {
        Filename = 0,
        Title,
        Artist,
        AlbumArtist,
        Composer,
        Year,
        Album,
        DiscNumber,
        Track,
        Bpm,
        Genre,
        Comment,
        Directory,
        Type,
        Length,
        Bitrate,
        SampleRate,
        Score,
        Rating,
        PlayCount,
        LastPlayed,
        Mood,
        Filesize,
        NUM_COLUMNS
    };

    class LIBAMAROK_EXPORT EmbeddedImage {
    public:
        EmbeddedImage() {}
        EmbeddedImage( const TagLib::ByteVector& data, const TagLib::String& description );

        const QCString &hash() const;
        const QString &description() const { return m_description; }
        bool save( const QDir& dir ) const;

    private:
        QByteArray m_data;
        QString m_description;
        mutable QCString m_hash;
   };

    typedef QValueList<EmbeddedImage> EmbeddedImageList;

    /** This is a bit vector for selecting columns. It's very fast to compare
        in matchFast. It might be a good idea to replace the QValue<int>
        column masks with this eventually. */
    typedef Q_UINT32 ColumnMask;

    /** Returns the name of the column at \p index as a string -- not i18ned, for internal purposes. */
    static const QString &exactColumnName( int index );
    /** Returns the name of the column at \p index as a string -- i18ned, for display purposes. */
    static const QString prettyColumnName( int index );
    /** Returns the index of the column with the not i18ned name \p name. */
    static int columnIndex( const QString &name );

    // These values are stored on the Database, so, don't change the order. Only append new ones to the end.
    enum FileType { other, mp3, ogg, wma, mp4, flac, ra, rv, rm, rmj, rmvb, asf };

    //for the audioproperties
    static const int Undetermined = -2; /// we haven't yet read the tags
    static const int Irrelevant   = -1; /// not applicable to this stream/media type, eg length for http streams
    static const int Unavailable  =  0; /// cannot be obtained

    // whether file is part of a compilation
    enum Compilation { CompilationNo = 0, CompilationYes = 1, CompilationUnknown = -1 };

    /// Creates an empty MetaBundle
    LIBAMAROK_EXPORT MetaBundle();

    /// Creates a MetaBundle for url, tags will be obtained and set
    LIBAMAROK_EXPORT explicit MetaBundle( const KURL &url,
                                          bool noCache = false,
                                          TagLib::AudioProperties::ReadStyle = TagLib::AudioProperties::Fast,
                                          EmbeddedImageList* images = 0 );

    /** For the StreamProvider */
    LIBAMAROK_EXPORT MetaBundle( const QString &title,
            const QString &streamUrl,
            const int bitrate,
            const QString &genre,
            const QString &streamName,
            const KURL &url );

    LIBAMAROK_EXPORT MetaBundle( const MetaBundle &bundle );

    ~MetaBundle();

    MetaBundle& operator=( const MetaBundle& bundle );
    bool operator==( const MetaBundle& bundle ) const;
    bool operator!=( const MetaBundle& bundle ) const;

    /** Test for an empty metabundle */
    bool isEmpty() const;

    /** Empty the metabundle */
    void clear();

    /** Is it media that has metadata? Note currently we don't check for an audio mimetype */
    bool isValidMedia() const;

    /** The bundle doesn't yet know its audioProperties */
    bool audioPropertiesUndetermined() const;

    /** The embedded artwork in the file (loaded from file into images variable, unmodified if no images present/loadable) */
    void embeddedImages(EmbeddedImageList &images) const;

    /** If you want Accurate reading say so. If EmbeddedImageList != NULL, embedded art is loaded into it */
    void readTags( TagLib::AudioProperties::ReadStyle = TagLib::AudioProperties::Fast, EmbeddedImageList* images = 0 );

    /** Saves the changes to the file using the transactional algorithm for safety. */
    bool safeSave();

    /** Saves the changes to the file. Returns false on error. */
    bool save( TagLib::FileRef* fileref = 0 );

    /** Saves the MetaBundle's data as XML to a text stream. */
    bool save( QTextStream &stream, const QStringList &attributes = QStringList() ) const;

    /** Returns whether the url referred to is a local file */
    bool isFile() const;

    /** Returns whether the url referred to can be accessed via kio slaves */
    bool isKioUrl() const;

    /** Returns whether url can be accessed via kio slaves */
    static bool isKioUrl( const KURL &url );

    /** Returns whether composer, disc number and bpm fields are available. */
    bool hasExtendedMetaInformation() const;

    void copyFrom( const MetaBundle& bundle );

    void copyFrom( const PodcastEpisodeBundle &peb );

    /** Returns a string representation of the tag at \p column, in a format suitable for internal purposes.
        For example, for a track 3:24 long, it'll return "204" (seconds).
        This should not be used for displaying the tag to the user. */
    QString exactText( int column, bool ensureCached = false ) const;

    /** Sets the tag at \p column from a string in the same format as returned by exactText(). */
    void setExactText( int column, const QString &text );

    /** Returns the tag at \p column in a format suitable for displaying to the user. */
    QString prettyText( int column ) const;

    /** Returns whether the bundle matches \p expression.
        This is fast and doesn't take advanced syntax into account,
        and should only be used when it is certain none is present.
        The tags in \p columns are checked for matches.
        @see ExpressionParser::isAdvancedExpression() */
    bool matchesSimpleExpression( const QString &expression, const QValueList<int> &columns ) const;

    /** A faster version of the above, that pre-caches all the data to be
        searched in a single string, to avoid re-building integer and lower
        case strings over and over. It is designed to be called from a
        playlist search *only* -- it is not entirely thread-safe for efficiency,
        although it's highly unlikely to crash. Consider this the beginning
        of a real super-efficient index (e.g. suffix tree).
        \p terms is a list of lower-case words. */
    bool matchesFast(const QStringList &terms, ColumnMask columns) const;

    /** Returns whether the bundle matches \p expression.
        This takes advanced syntax into account, and is slightly slower than matchesSimpleExpression().
        The tags in \p defaultColumns are checked for matches where the expression doesn't specify any manually. */
    bool matchesExpression( const QString &expression, const QValueList<int> &defaultColumns ) const;

    /** Returns whether the bundle matches the pre-parsed expression \p parsedData.
        The tags in \p defaultColumns are checked for matches where the expression doesn't specify any manually.
        @see ExpressionParser */
    bool matchesParsedExpression( const ParsedExpression &parsedData, const QValueList<int> &defaultColumns ) const;

    /** PlaylistItem reimplements this so it can be informed of moodbar
        data events without having to use signals */
    virtual void moodbarJobEvent( int newState )
        { (void) newState; }

public:
    /**
     * A class to load MetaBundles from XML.
     * #include "xmlloader.h"
     */
    class XmlLoader;

public: //accessors
    const KURL &url()               const;
    QString      title()     const;
    AtomicString artist()    const;
    AtomicString albumArtist() const;
    AtomicString composer()  const;
    AtomicString album()     const;
    AtomicString genre()     const;
    AtomicString comment()   const;
    QString      filename()  const;
    QString      directory() const;
    QString      type()      const;
    int     year()        const;
    int     discNumber()  const;
    int     track()       const;
    float   bpm()         const;
    int     length()      const;
    int     bitrate()     const;
    int     sampleRate()  const;
    float   score( bool ensureCached = false )      const;
    int     rating( bool ensureCached = false )     const; //returns rating * 2, to accommodate .5 ratings
    int     playCount( bool ensureCached = false )  const;
    uint    lastPlay( bool ensureCached = false )   const;

    Moodbar       &moodbar();
    const Moodbar &moodbar_const() const;

    int     filesize()    const;

    int compilation() const;
    int fileType() const;  // returns a value from enum FileType
    bool exists() const; // true for everything but local files that aren't there
    PodcastEpisodeBundle *podcastBundle() const;
    LastFm::Bundle *lastFmBundle() const;
    QString streamName() const;
    QString streamUrl()  const;
    QString uniqueId() const;

    QString prettyTitle() const;
    QString veryNiceTitle() const;
    QString prettyURL() const;
    QString prettyBitrate() const;
    QString prettyLength() const;
    QString prettySampleRate( bool shortened = false ) const;
    QString prettyFilesize() const;
    QString prettyRating() const;

    bool safeToSave() { return m_safeToSave; }

    QString getRandomString( int size, bool numbersOnly = false );

public: //modifiers
    void setUrl( const KURL &url );
    void setPath( const QString &path );
    void setTitle( const QString &title );
    void setArtist( const AtomicString &artist );
    void setAlbumArtist( const AtomicString &albumArtist );
    void setComposer( const AtomicString &composer );
    void setAlbum( const AtomicString &album );
    void setGenre( const AtomicString &genre );
    void setComment( const AtomicString &comment );
    void setYear( int year );
    void setDiscNumber( int discNumber );
    void setTrack( int track );
    void setBpm( float bpm );
    void setLength( int length );
    void setBitrate( int bitrate );
    void setSampleRate( int sampleRate );
    void setScore( float score );
    void setRating( int rating );
    void setPlayCount( int playcount );
    void setLastPlay( uint lastplay );
    void setFilesize( int bytes );
    // No direct moodbar mutator -- moodbar should not be separated
    // from the metabundle

    void updateFilesize();
    void setFileType( int type );
    void setCompilation( int compilation );
    bool checkExists();
    void setPodcastBundle( const PodcastEpisodeBundle &peb );
    void setLastFmBundle( const LastFm::Bundle &last );
    void setUniqueId(); //uses database for lookup
    void setUniqueId( const QString &id ); //SEE COMMENT in .CPP
    const TagLib::ByteVector readUniqueIdHelper( TagLib::FileRef fileref ) const;
    QString readUniqueId( TagLib::FileRef *fileref = 0  );
    void scannerAcknowledged() {}

    void detach(); // for being able to apply QDeepCopy<>

public: //static helper functions
    static QString prettyBitrate( int );
    static QString prettyLength( int, bool showHours = false ); //must be int, see Unavailable, etc. above
    static QString prettyFilesize( int );
    static QString prettyRating( int rating, bool trailingzero = false );
    static QString ratingDescription( int );
    static QStringList ratingList();
    static QString prettyTime( uint, bool showHours = true );
    static QString fuzzyTime( int );
    static QString veryPrettyTime( int );
    static QString zeroPad( uint i );
    static QString prettyTitle( const QString &filename );
    static QStringList genreList();

protected:
    enum ExtendedTags { composerTag, albumArtistTag, discNumberTag, bpmTag, compilationTag };

    /** Called before the tags in \p columns are changed. */
    virtual void aboutToChange( const QValueList<int> &columns );

    /** Convenience method. */
    void aboutToChange( int column );

    /** Called after the tags in \p columns are changed. */
    virtual void reactToChanges( const QValueList<int> &columns );

    /** Convenience method. */
    void reactToChange( int column );

    KURL m_url;
    QString m_title;
    AtomicString m_artist;
    AtomicString m_albumArtist;
    AtomicString m_composer;
    AtomicString m_album;
    AtomicString m_comment;
    AtomicString m_genre;
    QString m_streamName;
    QString m_streamUrl;
    QString m_uniqueId;

    int m_year;
    int m_discNumber;
    int m_track;
    float m_bpm;
    int m_bitrate;
    int m_length;
    int m_sampleRate;

    float m_score;
    int m_rating;
    int m_playCount;
    uint m_lastPlay;
    int m_filesize;

    Moodbar *m_moodbar;

    int m_type;

    bool m_exists: 1;
    bool m_isValidMedia: 1;
    bool m_isCompilation: 1;
    bool m_notCompilation: 1;
    bool m_safeToSave: 1;
    int m_waitingOnKIO;
    QString m_tempSavePath;
    QString m_origRenamedSavePath;
    QCString m_tempSaveDigest;
    TagLib::FileRef* m_saveFileref;

    PodcastEpisodeBundle *m_podcastBundle;
    LastFm::Bundle *m_lastFmBundle;

    // The vars below are used to optimize search by storing
    // the full text to be searched. They are mutable, as they
    // act like a sort of cache for the const method matchesFast

    // whether the search text should be rebuilt
    volatile mutable bool m_isSearchDirty;
    // which columns the search string contains
    mutable ColumnMask m_searchColumns;
    // the search string: textualized columns separated by space
    // note that matchFast searches by words, hence a word cannot span
    // space-separated columns
    mutable QString m_searchStr;
private:

    static inline QString prettyGeneric( const QString &s, const int i )
    {
        return (i > 0) ? s.arg( i ) : (i == Undetermined) ? "?" : "-";
    }

    void init( TagLib::AudioProperties *ap = 0 );
    void init( const KFileMetaInfo& info );

    void setExtendedTag( TagLib::File *file, int tag, const QString value );

    void loadImagesFromTag( const TagLib::ID3v2::Tag &tag, EmbeddedImageList& images ) const;

    int getRand();
};

/// for your convenience
typedef QValueList<MetaBundle> BundleList;



inline bool MetaBundle::operator!=(const MetaBundle &bundle) const { return !operator==( bundle ); }

inline bool MetaBundle::isEmpty() const { return url().isEmpty(); }

inline bool MetaBundle::isValidMedia() const { return m_isValidMedia; }

inline bool MetaBundle::audioPropertiesUndetermined() const
{
    return m_bitrate == Undetermined || m_sampleRate == Undetermined || m_length == Undetermined;
}

inline void MetaBundle::aboutToChange( const QValueList<int>& ) { }
inline void MetaBundle::aboutToChange( int column ) { aboutToChange( QValueList<int>() << column ); }
inline void MetaBundle::reactToChange( int column ) { reactToChanges( QValueList<int>() << column ); }

inline bool MetaBundle::exists() const { return m_exists; }

inline bool MetaBundle::isFile() const { return url().isLocalFile(); }
inline bool MetaBundle::isKioUrl() const { return isKioUrl( url() ); }
inline bool MetaBundle::isKioUrl( const KURL &url ) { return url.protocol() != "daap" && url.protocol() != "cdda" && url.protocol() != "lastfm"; }

inline int MetaBundle::track()      const { return m_track == Undetermined ? 0 : m_track; }
inline int MetaBundle::year()       const { return m_year  == Undetermined ? 0 : m_year; }
inline int MetaBundle::length()     const { return m_length > 0 ? m_length : 0; }
inline int MetaBundle::bitrate()    const { return m_bitrate == Undetermined ? 0 : m_bitrate; }
inline int MetaBundle::sampleRate() const { return m_sampleRate == Undetermined ? 0 : m_sampleRate; }
inline int MetaBundle::filesize()   const { return m_filesize == Undetermined ? 0 : m_filesize; }
inline int MetaBundle::fileType()   const { return m_type; }

inline Moodbar &MetaBundle::moodbar()
{
  if( m_moodbar == 0 ) m_moodbar = new Moodbar( this );
  return *m_moodbar;
}
inline const Moodbar &MetaBundle::moodbar_const() const
{
  // Anyone know of a better way to do this?
  if( m_moodbar == 0 )
    const_cast<MetaBundle*>(this)->m_moodbar
      = new Moodbar( const_cast<MetaBundle*>(this) );
  return *m_moodbar;
}

inline const KURL&     MetaBundle::url()        const { return m_url; }
inline QString  MetaBundle::filename()   const { return url().fileName(); }
inline QString  MetaBundle::directory()  const
{
    return url().isLocalFile() ? url().directory() : url().upURL().prettyURL();
}
inline QString MetaBundle::title()            const { return m_title; }
inline AtomicString MetaBundle::artist()      const { return m_artist; }
inline AtomicString MetaBundle::album()       const { return m_album; }
inline AtomicString MetaBundle::comment()     const { return m_comment; }
inline AtomicString MetaBundle::genre()       const { return m_genre; }
inline AtomicString MetaBundle::composer()    const { return m_composer; }
inline AtomicString MetaBundle::albumArtist() const { return m_albumArtist; }
inline QString MetaBundle::streamName() const { return m_streamName; }
inline QString MetaBundle::streamUrl()  const { return m_streamUrl; }
inline QString MetaBundle::uniqueId()   const { return m_uniqueId; }

inline int MetaBundle::discNumber() const { return m_discNumber == Undetermined ? 0 : m_discNumber; }
inline float MetaBundle::bpm() const { return m_bpm == Undetermined ? 0 : m_bpm; }
inline int MetaBundle::compilation() const
{
    if( m_isCompilation )
        return CompilationYes;
    else if( m_notCompilation )
        return CompilationNo;
    else
        return CompilationUnknown;
}


inline QString MetaBundle::type() const
{
    return isFile()
           ? filename().mid( filename().findRev( '.' ) + 1 )
           : i18n( "Stream" );
}
inline PodcastEpisodeBundle *MetaBundle::podcastBundle() const { return m_podcastBundle; }
inline LastFm::Bundle *MetaBundle::lastFmBundle() const { return m_lastFmBundle; }

inline QString MetaBundle::prettyURL() const { return url().prettyURL(); }
inline QString MetaBundle::prettyBitrate() const { return prettyBitrate( m_bitrate ); }
inline QString MetaBundle::prettyLength() const { return prettyLength( m_length, true ); }
inline QString MetaBundle::prettyFilesize() const { return prettyFilesize( filesize() ); }
inline QString MetaBundle::prettyRating() const { return prettyRating( rating() ); }
inline QString MetaBundle::prettySampleRate( bool shortened ) const
    {
        if ( shortened )
            return prettyGeneric( i18n( "SampleRate", "%1 kHz" ), m_sampleRate / 1000 );
        else
            return prettyGeneric( i18n( "SampleRate", "%1 Hz" ), m_sampleRate );
    }

inline QString MetaBundle::zeroPad( uint i ) { return ( i < 10 ) ? QString( "0%1" ).arg( i ) : QString::number( i ); }

inline bool MetaBundle::hasExtendedMetaInformation() const
{
    return ( m_type == mp3 || m_type == ogg ||
             m_type== mp4  || m_type == flac );
}


#endif

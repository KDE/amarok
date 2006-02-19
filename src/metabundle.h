// Max Howell <max.howell@methylblue.com>, (C) 2004
// Alexandre Pereira de Oliveira <aleprj@gmail.com>, (C) 2005
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
#include "atomicstring.h"
#include "atomicurl.h"

#include "amarok_export.h"

class KFileMetaInfo;
class QTextStream;
template<class T> class QValueList;
namespace TagLib {
    class File;
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
        Composer,
        Year,
        Album,
        DiscNumber,
        Track,
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

    /** Returns the name of the column at \p index as a string -- not i18ned, for internal purposes. */
    static const QString exactColumnName( int index );
    /** Returns the name of the column at \p index as a string -- i18ned, for display purposes. */
    static const QString prettyColumnName( int index );
    /** Returns the index of the column with the not i18ned name \p name. */
    static int columnIndex( const QString &name );

    // These values are stored on the Database, so, don't change the order. Only append new ones to the end.
    enum FileType { other, mp3, ogg, wma, mp4, flac, ra, rv, rm, rmj, rmvb };

    //for the audioproperties
    static const int Undetermined = -2; /// we haven't yet read the tags
    static const int Irrelevant   = -1; /// not applicable to this stream/media type, eg length for http streams
    static const int Unavailable  =  0; /// cannot be obtained

    /// Creates an empty MetaBundle
    LIBAMAROK_EXPORT MetaBundle();

    /// Creates a MetaBundle for url, tags will be obtained and set
    LIBAMAROK_EXPORT explicit MetaBundle( const KURL &url,
                                          bool noCache = false,
                                          TagLib::AudioProperties::ReadStyle = TagLib::AudioProperties::Fast );

    /** For the StreamProvider */
    LIBAMAROK_EXPORT MetaBundle( const QString &title,
            const QString &streamUrl,
            const int bitrate,
            const QString &genre,
            const QString &streamName,
            const KURL &url );

    bool operator==( const MetaBundle& bundle ) const;
    bool operator!=( const MetaBundle& bundle ) const;

    /** Test for an empty metabundle */
    bool isEmpty() const;

    /** Is it media that has metadata? Note currently we don't check for an audio mimetype */
    bool isValidMedia() const;

    /** The bundle doesn't yet know its audioProperties */
    bool audioPropertiesUndetermined() const;

    /** If you want Accurate reading say so */
    void readTags( TagLib::AudioProperties::ReadStyle );

    /** Saves the changes to the file. Returns false on error. */
    bool save();

    /** Saves the MetaBundle's data as XML to a text stream. */
    bool save( QTextStream &stream, const QStringList &attributes = QStringList(), int indent = 0 ) const;

    /** used by PlaylistItem, should be true for everything but local files that aren't there */
    bool exists() const;
    bool checkExists();

    bool isStream() const;

    /** Returns a value from enum FileType. */
    int fileType() const;

    /** Returns whether composer and disc number fields are available. */
    bool hasExtendedMetaInformation() const;

    void copyFrom( const MetaBundle& bundle );

    /** Returns a string representation of the tag at \p column, in a format suitable for internal purposes.
        For example, for a track 3:24 long, it'll return "204" (seconds).
        This should not be used for displaying the tag to the user. */
    QString exactText( int column ) const;

    /** Sets the tag at \p column from a string in the same format as returned by exactText(). */
    void setExactText( int column, const QString &text );

    /** Returns the tag at \p column in a format suitable for displaying to the user. */
    QString prettyText( int column ) const;

    /** Returns whether \p expression might contain advanced syntactical elements (OR, -, et al). */
    static bool isAdvancedExpression( const QString &expression );

    /** Returns whether the bundle matches \p expression.
        This is fast and doesn't take advanced syntax into account,
        and should only be used when it is certain none is present.
        The tags in \p columns are checked for matches. */
    bool matchesSimpleExpression( const QString &expression, QValueList<int> columns ) const;

    /** Returns whether the bundle matches \p expression.
        This takes advanced syntax into account, and is slightly slower than matchesSimpleExpression().
        The tags in \p defaultColumns are checked for matches where the expression doesn't specify any manually. */
    bool matchesExpression( const QString &expression, QValueList<int> defaultColumns ) const;

    /** The internal type used for an expression after it has been parsed. */
    typedef QValueList<QStringList> ParsedExpression;

    /** Parses \p expression into a format suitable for matchesParsedExpression().
        These functions are useful if you want to check many items, and only parse the expression once. */
    static ParsedExpression parseExpression( QString expression );

    /** Returns whether the bundle matches the pre-parsed expression \p parsedData.
        The tags in \p defaultColumns are checked for matches where the expression doesn't specify any manually. */
    bool matchesParsedExpression( ParsedExpression parsedData, QValueList<int> defaultColumns ) const;

public: //accessors
    KURL url()           const;

    QString      title()     const;
    AtomicString artist()    const;
    AtomicString composer()  const;
    AtomicString album()     const;
    AtomicString genre()     const;
    AtomicString comment()   const;
    QString      filename()  const;
    QString      directory() const;
    QString      type()      const;

    int     year()       const;
    int     discNumber() const;
    int     track()      const;
    int     length()     const;
    int     bitrate()    const;
    int     sampleRate() const;
    int     score()      const;
    int     rating()     const;
    int     playCount()  const;
    uint    lastPlay()   const;
    int     filesize()   const;

    QString streamName() const;
    QString streamUrl()  const;

    QString prettyTitle() const;
    QString veryNiceTitle() const;
    QString prettyURL() const;
    QString prettyBitrate() const;
    QString prettyLength() const;
    QString prettySampleRate( bool shortened = false ) const;
    QString prettyFilesize() const;
    QString prettyRating() const;

public: //modifiers
    virtual void setUrl( const KURL &url );
    virtual void setPath( const QString &path );
    virtual void setTitle( const QString &title );
    virtual void setArtist( const AtomicString &artist );
    virtual void setComposer( const AtomicString &composer );
    virtual void setAlbum( const AtomicString &album );
    virtual void setGenre( const AtomicString &genre );
    virtual void setComment( const AtomicString &comment );

    virtual void setYear( int year );
    virtual void setDiscNumber( int discNumber );
    virtual void setTrack( int track );
    virtual void setLength( int length );
    virtual void setBitrate( int bitrate );
    virtual void setSampleRate( int sampleRate );
    virtual void setScore( int score );
    virtual void setRating( int rating );
    virtual void setPlayCount( int playcount );
    virtual void setLastPlay( uint lastplay );
    virtual void setFilesize( int bytes );
    virtual void setFileType( int type );
    virtual void updateFilesize();

public: //static helper functions
    static QString prettyBitrate( int );
    static QString prettyLength( int, bool showHours = false ); //must be int, see Unavailable, etc. above
    static QString prettyFilesize( int );
    static QString prettyRating( int );
    static QStringList ratingList();
    static QString prettyTime( uint, bool showHours = true );
    static QString zeroPad( uint i );
    static QString prettyTitle( const QString &filename );
    static QStringList genreList();

protected:

    enum ExtendedTags { composerTag,  discNumberTag };

    KURL m_url;
//     AtomicURL m_url;
    QString m_title;
    AtomicString m_artist;
    AtomicString m_composer;
    AtomicString m_album;
    AtomicString m_comment;
    AtomicString m_genre;
    QString m_streamName;
    QString m_streamUrl;

    int m_year;
    int m_discNumber;
    int m_track;
    int m_bitrate;
    int m_length;
    int m_sampleRate;

    int  m_score;
    int  m_rating;
    int  m_playCount;
    uint m_lastPlay;
    int  m_filesize;

    int m_type;

    bool m_exists;
    bool m_isValidMedia;

private:

    static inline QString prettyGeneric( const QString &s, const int i )
    {
        return (i > 0) ? s.arg( i ) : (i == Undetermined) ? "?" : "-";
    }

    void init( TagLib::AudioProperties *ap = 0 );
    void init( const KFileMetaInfo& info );

    void setExtendedTag( TagLib::File *file, int tag, const QString value );
};

/// for your convenience
typedef QValueList<MetaBundle> BundleList;



inline bool MetaBundle::operator!=(const MetaBundle &bundle) const { return !operator==( bundle ); }

inline bool MetaBundle::isEmpty() const { return m_url.isEmpty(); }

inline bool MetaBundle::isValidMedia() const { return m_isValidMedia; }

inline bool MetaBundle::audioPropertiesUndetermined() const
{
    return m_bitrate == Undetermined || m_sampleRate == Undetermined || m_length == Undetermined;
}

inline bool MetaBundle::exists() const { return m_exists; }

inline bool MetaBundle::isStream() const { return url().protocol() == "http"; }

inline int MetaBundle::track()      const { return m_track == Undetermined ? 0 : m_track; }
inline int MetaBundle::year()       const { return m_year  == Undetermined ? 0 : m_year; }
inline int MetaBundle::length()     const { return m_length > 0 ? m_length : 0; }
inline int MetaBundle::bitrate()    const { return m_bitrate == Undetermined ? 0 : m_bitrate; }
inline int MetaBundle::sampleRate() const { return m_sampleRate == Undetermined ? 0 : m_sampleRate; }
inline int MetaBundle::filesize()   const { return m_filesize == Undetermined ? 0 : m_filesize; }
inline int MetaBundle::fileType()   const { return m_type; }

inline KURL     MetaBundle::url()        const { return m_url; }
inline QString  MetaBundle::filename()   const { return m_url.fileName(); }
inline QString  MetaBundle::directory()  const
{
    return m_url.isLocalFile() ? m_url.directory() : m_url.upURL().prettyURL();
}
inline QString MetaBundle::title()      const { return m_title; }
inline AtomicString MetaBundle::artist()     const { return m_artist; }
inline AtomicString MetaBundle::album()      const { return m_album; }
inline AtomicString MetaBundle::comment()    const { return m_comment; }
inline AtomicString MetaBundle::genre()      const { return m_genre; }
inline QString MetaBundle::streamName() const { return m_streamName; }
inline QString MetaBundle::streamUrl()  const { return m_streamUrl; }

inline int MetaBundle::discNumber() const { return m_discNumber == Undetermined ? 0 : m_discNumber; }

inline AtomicString MetaBundle::composer() const { return m_composer; }

inline QString MetaBundle::type() const
{
    return isStream()
           ? i18n( "Stream" )
           : filename().mid( filename().findRev( '.' ) + 1 );
}

inline QString MetaBundle::prettyURL() const { return m_url.prettyURL(); }
inline QString MetaBundle::prettyBitrate() const { return prettyBitrate( m_bitrate ); }
inline QString MetaBundle::prettyLength() const { return prettyLength( m_length, true ); }
inline QString MetaBundle::prettyFilesize() const { return prettyFilesize( m_filesize ); }
inline QString MetaBundle::prettyRating() const { return prettyRating( m_rating ); }
inline QString MetaBundle::prettySampleRate( bool shortened ) const
    {
        if ( shortened )
            return prettyGeneric( i18n( "SampleRate", "%1 kHz" ), m_sampleRate / 1000 );
        else
            return prettyGeneric( i18n( "SampleRate", "%1 Hz" ), m_sampleRate );
    }

inline QString MetaBundle::zeroPad( uint i ) { return ( i < 10 ) ? QString( "0%1" ).arg( i ) : QString::number( i ); }

inline void MetaBundle::setUrl( const KURL &url ) { m_url = url; }
inline void MetaBundle::setPath( const QString &path ) { m_url.setPath( path ); }
inline void MetaBundle::setTitle( const QString &title ) { m_title = title; }
inline void MetaBundle::setArtist( const AtomicString &artist ) { m_artist = artist; }
inline void MetaBundle::setAlbum( const AtomicString &album ) { m_album = album; }
inline void MetaBundle::setComment( const AtomicString &comment ) { m_comment = comment; }
inline void MetaBundle::setGenre( const AtomicString &genre ) { m_genre = genre; }
inline void MetaBundle::setYear( int year) { m_year = year; }
inline void MetaBundle::setTrack( int track ) { m_track = track; }
inline void MetaBundle::setLength( int length ) { m_length = length; }
inline void MetaBundle::setBitrate( int bitrate ) { m_bitrate = bitrate; }
inline void MetaBundle::setSampleRate( int sampleRate ) { m_sampleRate = sampleRate; }

inline void MetaBundle::setDiscNumber( int discnumber ) { m_discNumber = discnumber; }
inline void MetaBundle::setComposer( const AtomicString &composer ) { m_composer = composer; }

inline void MetaBundle::setPlayCount( int playcount ) { m_playCount = playcount; }
inline void MetaBundle::setLastPlay( uint lastplay ) { m_lastPlay = lastplay; }
inline void MetaBundle::setRating( int rating ) { m_rating = rating; }
inline void MetaBundle::setScore( int score ) { m_score = score; }
inline void MetaBundle::setFilesize( int bytes ) { m_filesize = bytes; }
inline void MetaBundle::setFileType( int type ) { m_type = type; }

inline bool MetaBundle::hasExtendedMetaInformation() const { return (m_type == mp3 || m_type == ogg || m_type== mp4 || m_type == flac ); }
#endif

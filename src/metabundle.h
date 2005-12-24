// Max Howell <max.howell@methylblue.com>, (C) 2004
// Alexandre Pereira de Oliveira <aleprj@gmail.com>, (C) 2005
// License: GNU General Public License V2

#ifndef METABUNDLE_H
#define METABUNDLE_H

#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
#define PRETTY_TITLE_CACHE
#endif

#include <kurl.h>    //inline functions
#include <klocale.h> //inline functions
#include <taglib/audioproperties.h>

class KFileMetaInfo;
class PlaylistItem;
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
 * @see class MutableBundle
 */

class MetaBundle
{
public:

    //for the audioproperties
    static const int Undetermined = -2; /// we haven't yet read the tags
    static const int Irrelevant   = -1; /// not applicable to this stream/media type, eg length for http streams
    static const int Unavailable  =  0; /// cannot be obtained

    enum FileType { mp3, ogg, wma, mp4, other };

    /**
     * Creates an empty MetaBundle
     */
    MetaBundle()
            : m_isValidMedia( false )
    {
        init();
    }

    /**
     * Creates a MetaBundle for url, tags will be obtained and set
     */
    explicit MetaBundle( const KURL&, bool noCache = false, TagLib::AudioProperties::ReadStyle = TagLib::AudioProperties::Fast );

    /** For the StreamProvider */
    MetaBundle( const QString &title,
            const QString &streamUrl,
            const int bitrate,
            const QString &genre,
            const QString &streamName,
            const KURL &url );

    /** For PlaylistItems */
    MetaBundle( const PlaylistItem *item );

    bool operator==( const MetaBundle& bundle );
    inline bool operator!=( const MetaBundle& bundle ) { return !operator==( bundle ); }


    /** Test for an empty metabundle */
    bool isEmpty() const { return m_url.isEmpty(); }

    /** Is it media that has metadata? Note currently we don't check for an audio mimetype */
    bool isValidMedia() const { return m_isValidMedia; }

    /** The bundle doesn't yet know its audioProperties */
    bool audioPropertiesUndetermined() const
    {
        return m_bitrate == Undetermined || m_sampleRate == Undetermined || m_length == Undetermined;
    }

    /** If you want Accurate reading say so */
    void readTags( TagLib::AudioProperties::ReadStyle );

    /** Saves the changes to the file. Returns false on error. */
    bool save();

    /** used by PlaylistItem, should be true for everything but local files that aren't there */
    bool exists() const { return true; }

    int track()      const { return m_track; }
    int year()       const { return m_year; }
    int length()     const { return m_length > 0 ? m_length : 0; }
    int bitrate()    const { return m_bitrate; }
    int sampleRate() const { return m_sampleRate; }

    const KURL    &url()        const { return m_url; }
          QString  filename()   const { return m_url.fileName(); }
          QString  directory()  const { return m_url.isLocalFile() ? m_url.directory()
                                                                   : m_url.upURL().prettyURL(); }
    const QString &title()      const { return m_title; }
    const QString &artist()     const { return m_artist; }
    const QString &album()      const { return m_album; }
    const QString &comment()    const { return m_comment; }
    const QString &genre()      const { return m_genre; }
    const QString &streamName() const { return m_streamName; }
    const QString &streamUrl()  const { return m_streamUrl; }

    int discNumber() const { return m_discNumber; }

    const QString &composer() const { return m_composer; }

    QString type( bool detectstream = true ) const
    {
        return ( detectstream && m_url.protocol() == "http" )
               ? i18n( "Stream" )
               : filename().mid( filename().findRev( '.' ) + 1 );
    }

    QString prettyTitle() const;
    QString veryNiceTitle() const;
    QString prettyURL() const { return m_url.prettyURL(); }
    QString prettyBitrate() const { return prettyBitrate( m_bitrate ); }
    QString prettyLength() const { return prettyLength( m_length, true ); }
    QString prettySampleRate( bool shortened = false ) const
    {
        if ( shortened )
            return prettyGeneric( i18n( "SampleRate", "%1 kHz" ), m_sampleRate / 1000 );
        else
            return prettyGeneric( i18n( "SampleRate", "%1 Hz" ), m_sampleRate );
    }

    QString infoByColumn( int column, bool pretty = false ) const;

    // these are helpful statics, don't use these in preference
    // to the ones above!
    static QString prettyBitrate( int );
    static QString prettyLength( int, bool showHours = false ); //must be int, see Unavailable, etc. above
    static QString prettyTime( uint, bool showHours = true );
    static QString zeroPad( uint i ) { return ( i < 10 ) ? QString( "0%1" ).arg( i ) : QString::number( i ); }
    static QString prettyTitle( const QString &filename );
    static QStringList genreList();

public:
    void setUrl( const KURL &url ) { m_url = url; }
    void setPath( const QString &path ) { m_url.setPath( path ); }
    void setTitle( const QString &title ) { m_title = title; }
    void setArtist( const QString &artist ) { m_artist = artist; }
    void setAlbum( const QString &album ) { m_album = album; }
    void setComment( const QString &comment ) { m_comment = comment; }
    void setGenre( const QString &genre ) { m_genre = genre; }
    void setYear( int year) { m_year = year; }
    void setTrack( int track ) { m_track = track; }
    void setLength( int length ) { m_length = length; }
    void setBitrate( int bitrate ) { m_bitrate = bitrate; }
    void setSampleRate( int sampleRate ) { m_sampleRate = sampleRate; }

    void setDiscNumber( int discnumber ) { m_discNumber = discnumber; }

    void setComposer( const QString &composer ) { m_composer = composer; }

    int fileType() { return m_type; }

    bool hasExtendedMetaInformation() const { return (m_type == mp3 || m_type == ogg); }

protected:

    enum ExtendedTags { composerTag,  discNumberTag };

    KURL m_url;
    QString m_title;
    QString m_artist;
    QString m_album;
    QString m_comment;
    QString m_genre;
    QString m_streamName;
    QString m_streamUrl;

    int m_year;
    int m_track;
    int m_bitrate;
    int m_length;
    int m_sampleRate;

    int m_discNumber;
    QString m_composer;

private:
    bool m_exists;
    bool m_isValidMedia;

    int m_type;

    static inline QString prettyGeneric( const QString &s, const int i )
    {
        return (i > 0) ? s.arg( i ) : (i == Undetermined) ? "?" : "-";
    }

    void init( TagLib::AudioProperties *ap = 0 );
    void init( const KFileMetaInfo& info );

    void checkExists();

    void setExtendedTag( TagLib::File *file, int tag, const QString value );
};


/// for your convenience
typedef QValueList<MetaBundle> BundleList;

#endif

// (c) 2006 Seb Ruiz <me@sebruiz.net>
// See COPYING file for licensing information

#ifndef AMAROK_PODCASTBUNDLE_H
#define AMAROK_PODCASTBUNDLE_H

#include "podcastsettings.h"
#include <kurl.h>

class PodcastChannelBundle
{
    public:
        PodcastChannelBundle() { }
        PodcastChannelBundle( const KURL &url, const QString &title, const QString &author, const KURL &link,
                              const QString &desc,  const QString &copy, PodcastSettings *settings )
        {   m_url = url;
            m_title = title;
            m_author = author;
            m_link = link;
            m_description = desc;
            m_copyright = copy;
            setSettings( settings );   }

        void setSettings( PodcastSettings *settings )
        {
            m_saveLocation = settings->saveLocation();
            m_autoscan     = settings->autoscan();
            m_fetchType    = settings->fetchType();
            m_autotransfer = settings->autoTransfer();
            m_purge        = settings->hasPurge();
            m_purgeCount   = settings->purgeCount();
        }

        PodcastSettings * getSettings()
        {
            return new PodcastSettings( m_title, m_saveLocation.url(), m_autoscan, m_fetchType, 
                                        m_autotransfer, m_purge, m_purgeCount );
        }

        /// Return the url of the podcast feed
        KURL    url()         const;
        /// The title of the Podcast channel
        QString title()       const;
        /// The author of the Podcast channel
        QString author()      const;
        /// A url to the webpage of the podcast
        KURL    link()        const;
        /// A url to the image of the podcast
        KURL    imageURL()        const;
        QString description() const;
        QString copyright()   const;
        /// The id which the parent folder has in the database
        int     parentId()    const;

        void    setURL( const KURL &u );
        void    setTitle( const QString &t );
        void    setAuthor( const QString &a );
        void    setLink( const KURL &l );
        void    setImageURL( const KURL &i );
        void    setDescription( const QString &d );
        void    setCopyright( const QString &c );
        void    setParentId( const int p );

        void    setSaveLocation( const KURL &s );
        void    setAutoScan( const bool b );
        void    setFetchType( const int i );
        void    setAutoTransfer( const bool b );
        void    setPurge( const bool b );
        void    setPurgeCount( const int i );

        //settings
        KURL saveLocation() const;
        bool autoscan()     const;
        int  fetchType()    const;
        bool autotransfer() const;
        bool hasPurge()     const;
        int  purgeCount()   const;

    private:
        KURL    m_url;
        QString m_title;
        QString m_author;
        KURL    m_link;
        KURL    m_imageUrl;
        QString m_description;
        QString m_copyright;
        int     m_parentId;

        KURL    m_saveLocation;
        bool    m_autoscan;
        int     m_fetchType;
        bool    m_autotransfer;
        bool    m_purge;
        int     m_purgeCount;
};

inline KURL    PodcastChannelBundle::url()         const { return m_url; }
inline QString PodcastChannelBundle::title()       const { return m_title; }
inline QString PodcastChannelBundle::author()      const { return m_author; }
inline KURL    PodcastChannelBundle::link()        const { return m_link; }
inline KURL    PodcastChannelBundle::imageURL()    const { return m_imageUrl; }
inline QString PodcastChannelBundle::description() const { return m_description; }
inline QString PodcastChannelBundle::copyright()   const { return m_copyright; }
inline int     PodcastChannelBundle::parentId()    const { return m_parentId; }

inline void    PodcastChannelBundle::setURL         ( const KURL &u )    { m_url = u; }
inline void    PodcastChannelBundle::setTitle       ( const QString &t ) { m_title = t; }
inline void    PodcastChannelBundle::setAuthor      ( const QString &a ) { m_author = a; }
inline void    PodcastChannelBundle::setLink        ( const KURL &l )    { m_link = l; }
inline void    PodcastChannelBundle::setImageURL    ( const KURL &i )    { m_imageUrl = i; }
inline void    PodcastChannelBundle::setDescription ( const QString &d ) { m_description = d; }
inline void    PodcastChannelBundle::setCopyright   ( const QString &c ) { m_copyright = c; }
inline void    PodcastChannelBundle::setParentId    ( const int p )      { m_parentId = p; }

inline void    PodcastChannelBundle::setSaveLocation( const KURL &s )    { m_saveLocation = s; }
inline void    PodcastChannelBundle::setAutoScan( const bool b )         { m_autoscan = b; }
inline void    PodcastChannelBundle::setFetchType( const int i )         { m_fetchType = i; }
inline void    PodcastChannelBundle::setAutoTransfer( const bool b )     { m_autotransfer = b; }
inline void    PodcastChannelBundle::setPurge( const bool b )            { m_purge = b; }
inline void    PodcastChannelBundle::setPurgeCount( const int i )        { m_purgeCount = i; }

inline KURL    PodcastChannelBundle::saveLocation() const { return m_saveLocation; }
inline bool    PodcastChannelBundle::autoscan()     const { return m_autoscan; }
inline int     PodcastChannelBundle::fetchType()    const { return m_fetchType; }
inline bool    PodcastChannelBundle::autotransfer() const { return m_autotransfer; }
inline bool    PodcastChannelBundle::hasPurge()     const { return m_purge; }
inline int     PodcastChannelBundle::purgeCount()   const { return m_purgeCount; }



class PodcastEpisodeBundle
{
    public:
        PodcastEpisodeBundle()
            : m_id( 0 )
            , m_duration( 0 )
            , m_isNew( false )
        {
        }
        PodcastEpisodeBundle( const KURL &url,       const KURL &parent,  const QString &title,
                              const QString &author, const QString &desc, const QString &date,
                              const QString &type,   const int duration,  const QString &guid,
                              const bool isNew  )
            : m_id( 0 )
        {
            m_url = url;
            m_parent = parent;
            m_author = author;
            m_title = title;
            m_description = desc;
            m_type = type;
            m_date = date;
            m_duration = duration < 0 ? 0 : duration;
            m_guid = guid;
            m_isNew = isNew;
        }


        /// The row id which this podcast episode has in the database
        int     dBId()        const;
        /// The remote url to the podcast episode
        KURL    url()         const;
        /// The local url of the podcast episode (if it has been downloaded, an invalid url otherwise)
        KURL    localUrl()    const;
        /// The url of the podcast channel
        KURL    parent()      const;
        QString author()      const;
        QString title()       const;
        QString subtitle()    const;
        QString description() const;
        QString date()        const;
        /// File type of the podcast episode, eg ogg, mp3 etc
        QString type()        const;
        int     duration()    const; // duration in seconds
        uint    size()        const; // file/stream size in bytes
        /// unique identifier that should be available in the feed (RSS 2.0: guid ATOM: id)
        QString guid()        const;
        /// Has this particular podcast episode been listened to?
        bool    isNew()       const;

        void    setDBId( const int i );
        void    setURL( const KURL &u );
        void    setLocalURL( const KURL &u );
        void    setParent( const KURL &u );
        void    setAuthor( const QString &a );
        void    setTitle( const QString &t );
        void    setSubtitle( const QString &s );
        void    setDescription( const QString &d );
        void    setDate( const QString &d );
        void    setType( const QString &t );
        void    setDuration( const int i );
        void    setSize( const uint i );
        void    setGuid( const QString &g );
        void    setNew( const bool &b );

    private:
        int     m_id;
        KURL    m_url;
        KURL    m_localUrl;
        KURL    m_parent;
        QString m_author;
        QString m_title;
        QString m_subtitle;
        QString m_description;
        QString m_date;
        QString m_type;
        int     m_duration;
        uint    m_size;
        QString m_guid;
        bool    m_isNew;
};

inline int     PodcastEpisodeBundle::dBId()        const { return m_id; }
inline KURL    PodcastEpisodeBundle::url()         const { return m_url; }
inline KURL    PodcastEpisodeBundle::localUrl()    const { return m_localUrl; }
inline KURL    PodcastEpisodeBundle::parent()      const { return m_parent; }
inline QString PodcastEpisodeBundle::author()      const { return m_author; }
inline QString PodcastEpisodeBundle::title()       const { return m_title; }
inline QString PodcastEpisodeBundle::subtitle()    const { return m_subtitle; }
inline QString PodcastEpisodeBundle::description() const { return m_description; }
inline QString PodcastEpisodeBundle::date()        const { return m_date; }
inline QString PodcastEpisodeBundle::type()        const { return m_type; }
inline int     PodcastEpisodeBundle::duration()    const { return m_duration; }
inline uint    PodcastEpisodeBundle::size()        const { return m_size; }
inline QString PodcastEpisodeBundle::guid()        const { return m_guid; }
inline bool    PodcastEpisodeBundle::isNew()       const { return m_isNew; }

inline void    PodcastEpisodeBundle::setDBId( const int i )             { m_id = i; }
inline void    PodcastEpisodeBundle::setURL( const KURL &u )            { m_url = u; }
inline void    PodcastEpisodeBundle::setLocalURL( const KURL &u )       { m_localUrl = u; }
inline void    PodcastEpisodeBundle::setParent( const KURL &u )         { m_parent = u; }
inline void    PodcastEpisodeBundle::setAuthor( const QString &a )      { m_author = a; }
inline void    PodcastEpisodeBundle::setTitle( const QString &t )       { m_title = t; }
inline void    PodcastEpisodeBundle::setSubtitle( const QString &t )    { m_subtitle = t; }
inline void    PodcastEpisodeBundle::setDescription( const QString &d ) { m_description = d; }
inline void    PodcastEpisodeBundle::setDate( const QString &d )        { m_date = d; }
inline void    PodcastEpisodeBundle::setType( const QString &t )        { m_type = t; }
inline void    PodcastEpisodeBundle::setDuration( const int i )         { m_duration = i; }
inline void    PodcastEpisodeBundle::setSize( const uint i )            { m_size = i; }
inline void    PodcastEpisodeBundle::setGuid( const QString &g )        { m_guid = g; }
inline void    PodcastEpisodeBundle::setNew( const bool &b )            { m_isNew = b; }

#endif /* AMAROK_PODCASTBUNDLE_H */

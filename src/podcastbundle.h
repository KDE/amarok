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
        PodcastChannelBundle( const KURL &url, const QString &title, const KURL &link,
                              const QString &desc,  const QString &copy, PodcastSettings *settings )
        {   m_url = url;
            m_title = title;
            m_link = link;
            m_description = desc;
            m_copyright = copy;
            setSettings( settings );   }
            
        void setSettings( PodcastSettings *settings )
        {
            m_saveLocation = settings->saveLocation();
            m_autoScan = settings->hasAutoScan();
            m_fetch = settings->fetch();
            m_autotransfer = settings->addToMediaDevice();
            m_purge = settings->hasPurge();
            m_purgeCount = settings->purgeCount();
        }
                              
        KURL    url()         const;
        QString title()       const;
        KURL    link()        const;
        QString description() const;
        QString copyright()   const;
        
        void    setURL( const KURL &u );
        void    setTitle( const QString &t );
        void    setLink( const KURL &l );
        void    setDescription( const QString &d );
        void    setCopyright( const QString &c );
        void    setSaveLocation( const KURL &s );
        void    setAutoScan( const bool b );
        void    setFetchType( const int i );
        void    setAutoTransfer( const bool b );
        void    setPurge( const bool b );
        void    setPurgeCount( const int i );
        
        //settings
        const KURL saveLocation() const;
        const bool autoscan()     const;
        const int  fetch()        const;
        const bool autotransfer() const;
        const bool hasPurge()     const;
        const int  purgeCount()   const;
        
    private:
        KURL    m_url;
        QString m_title;
        KURL    m_link;
        QString m_description;
        QString m_copyright;
        
        KURL    m_saveLocation;
        bool    m_autoScan;
        int     m_fetch;
        bool    m_autotransfer;
        bool    m_purge;
        int     m_purgeCount;
};

inline KURL    PodcastChannelBundle::url()         const { return m_url; }
inline QString PodcastChannelBundle::title()       const { return m_title; }
inline KURL    PodcastChannelBundle::link()        const { return m_link; }
inline QString PodcastChannelBundle::description() const { return m_description; }
inline QString PodcastChannelBundle::copyright()   const { return m_copyright; }

inline void    PodcastChannelBundle::setURL         ( const KURL &u )    { m_url = u; }
inline void    PodcastChannelBundle::setTitle       ( const QString &t ) { m_title = t; }
inline void    PodcastChannelBundle::setLink        ( const KURL &l )    { m_link = l; }
inline void    PodcastChannelBundle::setDescription ( const QString &d ) { m_description = d; }
inline void    PodcastChannelBundle::setCopyright   ( const QString &c ) { m_copyright = c; }

inline void    PodcastChannelBundle::setSaveLocation( const KURL &s )    { m_saveLocation = s; }
inline void    PodcastChannelBundle::setAutoScan( const bool b )         { m_autoScan = b; }
inline void    PodcastChannelBundle::setFetchType( const int i )         { m_fetch = i; }
inline void    PodcastChannelBundle::setAutoTransfer( const bool b )     { m_autotransfer = b; }
inline void    PodcastChannelBundle::setPurge( const bool b )            { m_purge = b; }
inline void    PodcastChannelBundle::setPurgeCount( const int i )        { m_purgeCount = i; }

inline const KURL PodcastChannelBundle::saveLocation() const { return m_saveLocation; }
inline const bool PodcastChannelBundle::autoscan()     const { return m_autoScan; }
inline const int  PodcastChannelBundle::fetch()        const { return m_fetch; }
inline const bool PodcastChannelBundle::autotransfer() const { return m_autotransfer; }
inline const bool PodcastChannelBundle::hasPurge()     const { return m_purge; }
inline const int  PodcastChannelBundle::purgeCount()   const { return m_purgeCount; }



class PodcastEpisodeBundle
{
    public:
        PodcastEpisodeBundle() { }
        PodcastEpisodeBundle( const KURL &url,       const KURL &parent,  const QString &title,
                              const QString &author, const QString &desc, const QString &date,
                              const QString &type,   const int duration,  const QString &guid,
                              const bool isNew  )
        {   
            m_url = url;
            m_parent = parent;
            m_title = title;
            m_author = author;
            m_description = desc;
            m_type = type;
            m_date = date;
            m_duration = duration < 0 ? 0 : duration;
            m_guid = guid;
            m_isNew = isNew; 
        }
        
        KURL    url()         const;
        KURL    parent()      const;
        QString author()      const;
        QString title()       const;
        QString description() const;
        QString date()        const;
        QString type()        const;
        int     duration()    const;
        QString guid()        const;
        bool    isNew()       const;
        
        void    setURL( const KURL &u );
        void    setParent( const KURL &u );
        void    setAuthor( const QString &a );
        void    setTitle( const QString &t );
        void    setDescription( const QString &d );
        void    setDate( const QString &d );
        void    setType( const QString &t );
        void    setDuration( const int i );
        void    setGuid( const QString &g );
        void    setNew( const bool &b );
        
    private:
        KURL    m_url;
        KURL    m_parent;
        QString m_author;
        QString m_title;
        QString m_description;
        QString m_date;
        QString m_type;
        int     m_duration;
        QString m_guid; //unique identifier that should be available in the feed (RSS 2.0: guid ATOM: id)
        bool    m_isNew;
};

inline KURL    PodcastEpisodeBundle::url()         const { return m_url; }
inline KURL    PodcastEpisodeBundle::parent()      const { return m_parent; }
inline QString PodcastEpisodeBundle::author()      const { return m_author; }
inline QString PodcastEpisodeBundle::title()       const { return m_title; }
inline QString PodcastEpisodeBundle::description() const { return m_description; }
inline QString PodcastEpisodeBundle::date()        const { return m_date; }
inline QString PodcastEpisodeBundle::type()        const { return m_type; }
inline int     PodcastEpisodeBundle::duration()    const { return m_duration; }
inline QString PodcastEpisodeBundle::guid()        const { return m_guid; }
inline bool    PodcastEpisodeBundle::isNew()       const { return m_isNew; }

inline void    PodcastEpisodeBundle::setURL( const KURL &u )            { m_url = u; }
inline void    PodcastEpisodeBundle::setParent( const KURL &u )         { m_parent = u; }
inline void    PodcastEpisodeBundle::setAuthor( const QString &a )      { m_author = a; }
inline void    PodcastEpisodeBundle::setTitle( const QString &t )       { m_title = t; }
inline void    PodcastEpisodeBundle::setDescription( const QString &d ) { m_description = d; }
inline void    PodcastEpisodeBundle::setDate( const QString &d )        { m_date = d; }
inline void    PodcastEpisodeBundle::setType( const QString &t )        { m_type = t; }
inline void    PodcastEpisodeBundle::setDuration( const int i )         { m_duration = i; }
inline void    PodcastEpisodeBundle::setGuid( const QString &g )        { m_guid = g; }
inline void    PodcastEpisodeBundle::setNew( const bool &b )            { m_isNew = b; }

#endif /* AMAROK_PODCASTBUNDLE_H */

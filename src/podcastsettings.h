// (c) 2005 Seb Ruiz <me@sebruiz.net>
// See COPYING file for licensing information.

#ifndef AMAROK_PODCASTSETTINGS_H
#define AMAROK_PODCASTSETTINGS_H

#include "kdialogbase.h"    //baseclass

#include <kurl.h>

class PodcastChannel;
class PodcastItem;
class PodcastSettingsDialogBase;
class QDomNode;
class QDomElement;

enum MediaFetch{ STREAM=0, AUTOMATIC=1 };

class PodcastSettings
{
    public:
        PodcastSettings( const QDomNode &channelSettings, const QString &title );
        PodcastSettings( const PodcastSettings *parentSettings, const QString &title );
        PodcastSettings( const QString &title ); // standard settings


        const KURL &url()         { return m_url; }
        const KURL &saveLocation(){ return m_saveLocation; }
        bool    hasAutoScan() { return m_autoScan; }
        int     interval()    { return m_interval; }
        int     fetch()       { return m_fetch; }
        bool    addToMediaDevice() { return m_addToMediaDevice; }
        bool    hasPurge()    { return m_purge; }
        int     purgeCount()  { return m_purgeCount; }

        const QDomElement xml();

        QString m_title;    //the title of the podcast or category these settings belong to
        KURL m_url;
        KURL m_saveLocation;
        bool    m_autoScan;
        int     m_interval;
        int     m_fetch;
        bool    m_addToMediaDevice;
        bool    m_purge;
        int     m_purgeCount;
};


class PodcastSettingsDialog : public KDialogBase
{
    Q_OBJECT

    public:
        PodcastSettingsDialog( PodcastSettings *settings, PodcastSettings *parentSettings, QWidget* parent = 0 );

        bool configure();

    protected:
        bool    hasChanged();

    protected slots:
        void    checkModified();
        void    slotOk();
        void    slotUser1();

    private:
        void setSettings( PodcastSettings *settings, bool changeSaveLocation );
        QString requesterSaveLocation();

        PodcastSettingsDialogBase *m_ps;
        PodcastSettings *m_settings;
        PodcastSettings *m_parentSettings;
};

#endif /*AMAROK_PODCASTSETTINGS_H*/

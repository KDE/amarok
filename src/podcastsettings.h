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
        PodcastSettings( const QString &url, const QString &save, bool autoScan, int interval,
                         int fetch, bool addToMediaDevice, bool purge, int purgeCount );

        PodcastSettings( const QDomNode &channelSettings );
        PodcastSettings(); // standard settings


        QString url()         { return m_url; }
        QString saveLocation(){ return m_save; }
        bool    hasAutoScan() { return m_autoScan; }
        int     interval()    { return m_interval; }
        int     fetch()       { return m_fetch; }
        bool    addToMediaDevice() { return m_addToMediaDevice; }
        bool    hasPurge()    { return m_purge; }
        int     purgeCount()  { return m_purgeCount; }

        const QDomElement xml();

        QString m_url;
        QString m_save;
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

    protected:
        bool    hasChanged();

    protected slots:
        void    checkModified();
        void    slotOk();
        void    slotUser1();

    private:
        QString requesterSaveLocation();

        PodcastSettingsDialogBase *m_ps;
        PodcastSettings *m_settings;
        PodcastSettings *m_parentSettings;
};

#endif /*AMAROK_PODCASTSETTINGS_H*/

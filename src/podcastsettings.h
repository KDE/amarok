// (c) 2005 Seb Ruiz <me@sebruiz.net>
// See COPYING file for licensing information.

#ifndef AMAROK_PODCASTSETTINGS_H
#define AMAROK_PODCASTSETTINGS_H

#include "kdialogbase.h"    //baseclass

#include <kurl.h>

class PodcastChannel;
class PodcastItem;
class PodcastSettingsDialogBase;

class PodcastSettings : public KDialogBase
{
    Q_OBJECT

    public:
        PodcastSettings( const QString &url, const QString &save, bool autoScan, int interval,
                         int fetch, bool purge, int purgeCount, QWidget* parent = 0 );



        QString url()         { return m_url; }
        QString saveLocation(){ return m_save; }
        bool    hasAutoScan() { return m_autoScan; }
        int     interval()    { return m_interval; }
        int     fetch()       { return m_fetch; }
        bool    hasPurge()    { return m_purge; }
        int     purgeCount()  { return m_purgeCount; }

    protected:
        bool    hasChanged();

    protected slots:
        void    slotOk();
        void    checkModified();

    private:
        enum MediaFetch{ STREAM=0, DOWNLOAD=1, AVAILABLE=2 };

        PodcastSettingsDialogBase *m_ps;

        QString m_url;
        QString m_save;
        bool    m_autoScan;
        int     m_interval;
        int     m_fetch;
        bool    m_purge;
        int     m_purgeCount;

};

#endif /*AMAROK_PODCASTSETTINGS_H*/

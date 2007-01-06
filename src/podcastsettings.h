// (c) 2005 Seb Ruiz <me@sebruiz.net>
// See COPYING file for licensing information.

#ifndef AMAROK_PODCASTSETTINGS_H
#define AMAROK_PODCASTSETTINGS_H

#include "kdialogbase.h"    //baseclass

#include <kurl.h>

#include <qptrlist.h>

class PodcastChannel;
class PodcastEpisode;
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
        PodcastSettings( const QString &title, const QString &save, const bool autoScan,
                         const int fetchType, const bool autotransfer, const bool purge, const int purgecount );

        const QString &saveLocation() { return m_saveLocation; }
        const QString &title() { return m_title; }
        bool    autoscan()         { return m_autoScan; }
        int     fetchType()        { return m_fetch; }
        bool    autoTransfer()     { return m_addToMediaDevice; }
        bool    hasPurge()         { return m_purge; }
        int     purgeCount()       { return m_purgeCount; }

        QString m_title;    //the title of the podcast or category these settings belong to
        QString m_saveLocation;
        bool    m_autoScan;
        int     m_fetch;
        bool    m_addToMediaDevice;
        bool    m_purge;
        int     m_purgeCount;
};


class PodcastSettingsDialog : public KDialogBase
{
    Q_OBJECT

    public:
        PodcastSettingsDialog( PodcastSettings *list, QWidget* parent=0 );
        PodcastSettingsDialog( const QPtrList<PodcastSettings> &list, const QString &caption, QWidget* parent=0 );

        bool    configure();
        PodcastSettings *getSettings() { return m_settings; }

    protected:
        bool    hasChanged();

    protected slots:
        void    checkModified();
        void    slotOk();
        void    slotUser1();

    private:
        void init();
        void setSettings( PodcastSettings *settings );
        QString requesterSaveLocation();

        PodcastSettingsDialogBase *m_ps;
        QPtrList<PodcastSettings> m_settingsList;
        PodcastSettings           *m_settings;
};

#endif /*AMAROK_PODCASTSETTINGS_H*/

/***************************************************************************
 * copyright            : (C) 2007 Shane King <kde@dontletsstart.com>      *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef LASTFMSETTINGS_H
#define LASTFMSETTINGS_H

#include <lastfm/radio/RadioStation.h>

#include <KConfigGroup>

#include <QString>

// provide the settings the code from last.fm client requires

class LastFmUserSettings
{
public:
    LastFmUserSettings();
    virtual ~LastFmUserSettings();

    void setDiscovery( bool discovery );
    bool isDiscovery() const;

    void setResumeStation( RadioStation station );
    RadioStation resumeStation() const;

    void addRecentStation( const class RadioStation& );

protected:
    KConfigGroup m_config;

private:
    RadioStation m_resumeStation;
};

class LastFmSettings : public LastFmUserSettings
{
public:
    LastFmUserSettings &currentUser() { return *this; } // only support single user
    QString currentUsername();

    QString appLanguage() const;

    void setFingerprintUploadUrl( const QString &url );

    QString version() const;

    // SharedSettings
    static LastFmSettings *instance();

    bool isUseProxy() const;
    QString getProxyHost() const;
    int getProxyPort() const;
    QString getProxyUser() const;
    QString getProxyPassword() const;
};

typedef LastFmSettings SharedSettings;

namespace The
{
    LastFmSettings &settings();
    inline LastFmUserSettings &currentUser() { return settings().currentUser(); }
    inline QString currentUsername() { return settings().currentUsername(); }
}

#endif // LASTFMSETINGS_H

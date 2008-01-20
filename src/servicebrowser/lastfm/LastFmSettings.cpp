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

#include "LastFmSettings.h"
#include "amarok.h"
#include "LastFmServiceConfig.h"
#include "UnicornCommon.h"

#include <KGlobal>
#include <KSharedConfig>

#include <QLocale>


LastFmUserSettings::LastFmUserSettings()
{
    m_config = KGlobal::config()->group( LastFmServiceConfig::configSectionName() );
}


void 
LastFmUserSettings::setDiscovery( bool discovery )
{
    m_config.writeEntry( "discovery", discovery );
}


bool 
LastFmUserSettings::isDiscovery() const
{
    return m_config.readEntry( "discovery", false );
}


void 
LastFmUserSettings::setResumeStation( StationUrl station )
{
    m_resumeStation = station; // don't save across sessions
}


StationUrl 
LastFmUserSettings::resumeStation() const
{
     return m_resumeStation; // don't save across sessions
}


void 
LastFmUserSettings::addRecentStation( const class Station& )
{
    // TODO
}


QString 
LastFmSettings::currentUsername()
{
    return LastFmServiceConfig().username();
}

QString
LastFmSettings::appLanguage() const
{
    QLocale::Language qtLang = QLocale::system().language();

    return UnicornUtils::qtLanguageToLfmLangCode( qtLang );
}


void 
LastFmSettings::setFingerprintUploadUrl( const QString &url )
{
    // todo
}


QString 
LastFmSettings::version() const
{
    return APP_VERSION;
}


// SharedSettings
LastFmSettings *
LastFmSettings::instance()
{
    return &The::settings();
}


bool 
LastFmSettings::isUseProxy() const
{
    return false; // TODO
}


QString 
LastFmSettings::getProxyHost() const
{
    return ""; // TODO
}


int 
LastFmSettings::getProxyPort() const
{
    return 0; // TODO
}


QString 
LastFmSettings::getProxyUser() const
{
    return ""; // TODO
}


QString 
LastFmSettings::getProxyPassword() const
{
    return ""; // TODO
}


namespace The
{
    LastFmSettings &settings()
    {
        static LastFmSettings settings;
        return settings;
    }
}

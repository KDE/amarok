/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Last.fm Ltd <client@last.fm>                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef SETTINGS_H
#define SETTINGS_H

#include "UnicornDllExportMacro.h"

#include "UnicornCommon.h"
#include "StationUrl.h"

#include <QMap>
#include <QPoint>
#include <QSettings>
#include <QSize>
#include <QStringList>
#include <QMutex>
#include <QMutexLocker>
#include <QCoreApplication>

/**
 * Settings that apply to all applications we are likely to write. Not stored
 * on a per-user basis.
 *
 * NOTE don't confuse this with UserSettings!
 */
template <typename T>
class UsersSettings : public T
{
public:
    UsersSettings( QObject* parent = 0 ) : T( parent )
    {
        T::beginGroup( "Users" );
    }
};


/**
 * The sole purpose of this class is to define the signal which requires a
 * Q_OBJECT macro. We want the subclass to be templated and templated classes
 * can't have signals.
 */
class UNICORN_DLLEXPORT UserSettingsBase : public QObject
{
    Q_OBJECT
    
signals:
    void userChanged( QString username );
};


template <typename T>
class UserSettings : public UserSettingsBase
{
public:
    UserSettings( const QString& username ) : m_username( username ) { }

    bool isNull() const { return m_username.isEmpty(); }

    QString username() const { return m_username; }

    QString password() const { return MyQSettings( this ).value( "Password" ).toString(); }
    void setPassword( QString password )
    {
        if ( !password.isEmpty() && password != "********" )
        {
            password = UnicornUtils::md5Digest( password.toUtf8() );
            MyQSettings( this ).setValue( "Password", password );
            emit userChanged( username() );
        }
    }

    bool rememberPass() const
    { 
        // Written as int for backwards compatibility with the MFC Audioscrobbler
        return (bool)MyQSettings( this ).value( "RememberPass", true ).toInt();
    }

    void setRememberPass( bool remember )
    {
        MyQSettings( this ).setValue( "RememberPass", int(remember) );
        emit userChanged( username() );
    }

protected:
    class MyQSettings : public UsersSettings<T>
    {
    public:
        MyQSettings( const UserSettings* const s )
        {
            beginGroup( s->username() );
        }
    };

private:
    QString m_username;
};


/**
 * Settings where the actual data is shared between all applications we are
 * likely to write. One example is proxy settings. These will be stored under
 * Last.fm/OrganizationDefaults.
 */
class UNICORN_DLLEXPORT SharedSettings : public QObject
{
public:

    #define SharedQSettings() QSettings( QCoreApplication::organizationName().isEmpty() ? "Last.fm" : QCoreApplication::organizationName() )

    SharedSettings( QObject* parent ) : QObject( parent ) { }

    bool isUseProxy()    const { return SharedQSettings().value( "ProxyEnabled" ).toInt() == 1; }
    void setUseProxy( bool v ) { SharedQSettings().setValue( "ProxyEnabled", v ? "1" : "0" ); }

    QString getProxyHost()      const { return SharedQSettings().value( "ProxyHost" ).toString(); }
    void    setProxyHost( QString v ) { SharedQSettings().setValue( "ProxyHost", v ); }

    int  getProxyPort()  const { return SharedQSettings().value( "ProxyPort" ).toInt(); }
    void setProxyPort( int v ) { SharedQSettings().setValue( "ProxyPort", v ); }

    QString getProxyUser()      const { return SharedQSettings().value( "ProxyUser" ).toString(); }
    void    setProxyUser( QString v ) { SharedQSettings().setValue( "ProxyUser", v ); }

    QString getProxyPassword()      const { return SharedQSettings().value( "ProxyPassword" ).toString(); }
    void    setProxyPassword( QString v ) { SharedQSettings().setValue( "ProxyPassword", v ); }

protected:

    /// This must be initialised with an instance of the relevant subclass
    /// inheriting from us at construction time.
    static SharedSettings* s_instance;

private:

    /**
     * This is a rather quirky scheme for making the Settings instance available
     * to Unicorn-level classes. It relies on an instance of the Settings having
     * been created by the application using it. If not, this function will
     * assert.
     *
     * We're making it private so that it's clear it's not meant for use outside
     * of the Unicorn lib. Classes that want to use it will need to befriend this
     * class.
     */
    static SharedSettings* instance()
    {
        Q_ASSERT( s_instance != 0 );
        return s_instance;
    }

    friend class CachedHttp;
    friend class WebService;

};


/**
 * Settings that apply to all applications we are likely to write but where the
 * actual data is different for each application. This is to be viewed as a
 * repository for shared settings-related code. Not stored on a per-user basis.
 *
 * Inherit from this class when writing your own app.
 *
 * Parameterised on the QSettings object type to use so that a subclass can tell
 * us what the correct storage location is.
 */
template <typename T>
class AppSettings : public SharedSettings
{
public:
    typedef T QSettings;

    AppSettings( QObject* parent ) : SharedSettings( parent ) { }

    QString path() const { return QSettings().value( "Path" ).toString(); }
    void setPath( QString p ) { QSettings().setValue( "Path", p ); }

    QString version() const
    {
        // ask Erik if you wonder why we store this in the configuration
        return QSettings().value( "Version", "unknown" ).toString();
    }
    void setVersion( QString v ) { QSettings().setValue( "Version", v ); }

    QByteArray containerGeometry() const { return QSettings().value( "MainWindowGeometry" ).toByteArray(); }
    void setContainerGeometry( QByteArray state ) { QSettings().setValue( "MainWindowGeometry", state ); }

    Qt::WindowState containerWindowState() const { return (Qt::WindowState) QSettings().value( "MainWindowState" ).toInt(); }
    void setContainerWindowState( int state ) { QSettings().setValue( "MainWindowState", state ); }

    /// Use one of our pre-defined 2-letter language codes
    QString appLanguage() const
    {
        QString langCode = customAppLanguage();
        if ( langCode.isEmpty() )
        {
            // If none found, use system locale
            #ifdef Q_WS_MAC
            QLocale::Language qtLang = UnicornUtils::osxLanguageCode();
            #else
            QLocale::Language qtLang = QLocale::system().language();
            #endif

            langCode = UnicornUtils::qtLanguageToLfmLangCode( qtLang );
        }

        return langCode;
    }

    QString customAppLanguage() const { return QSettings().value( "AppLanguage" ).toString(); }
    void setAppLanguage( QString langCode ) { QSettings().setValue( "AppLanguage", langCode ); }

};

#endif

/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMPACHESERVICE_H
#define AMPACHESERVICE_H

#include "AmpacheServiceCollection.h"
#include "ServiceBase.h"

#include <QPointer>

class AmpacheAccountLogin;

class AmpacheServiceFactory: public ServiceFactory
{
    Q_PLUGIN_METADATA(IID AmarokPluginFactory_iid FILE "amarok_service_ampache.json")
    Q_INTERFACES(Plugins::PluginFactory)
    Q_OBJECT

    public:
        AmpacheServiceFactory();
        virtual ~AmpacheServiceFactory() {}

        virtual bool possiblyContainsTrack( const QUrl &url ) const;

        virtual void init();
        virtual QString name();
        virtual KConfigGroup config();
};


/**
A service for displaying, previewing and downloading music from Ampache music servers

	@author
*/
class AmpacheService : public ServiceBase
{

Q_OBJECT
    
public:
    explicit AmpacheService( AmpacheServiceFactory* parent, const QString &name,
                             const QString &url = QString(), const QString &username = QString(),
                             const QString &password = QString() );

    ~AmpacheService();

    void polish();
    void reauthenticate();

    virtual Collections::Collection * collection() { return m_collection; }

private Q_SLOTS:
    void onLoginSuccessful();

private:
    InfoParserBase *m_infoParser;
    Collections::AmpacheServiceCollection *  m_collection;
    QPointer<AmpacheAccountLogin> m_ampacheLogin;

    // Disable copy constructor and assignment
    AmpacheService( const AmpacheService& );
    AmpacheService& operator=( const AmpacheService& );
};

#endif

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

#ifndef JAMENDOSERVICE_H
#define JAMENDOSERVICE_H

#include "ServiceBase.h"
#include "JamendoDatabaseHandler.h"
#include "JamendoXmlParser.h"
#include "ServiceSqlCollection.h"

#include "core/support/Amarok.h"
#include <kio/job.h>

class JamendoServiceFactory : public ServiceFactory
{
    Q_PLUGIN_METADATA(IID AmarokPluginFactory_iid FILE "amarok_service_jamendo.json")
    Q_INTERFACES(Plugins::PluginFactory)
    Q_OBJECT

    public:
        JamendoServiceFactory();
        virtual ~JamendoServiceFactory() {}

        virtual void init();
        virtual QString name();
        virtual KConfigGroup config();

        virtual bool possiblyContainsTrack( const QUrl &url ) const { return url.url().contains( "jamendo.com", Qt::CaseInsensitive ); }
};

class JamendoService : public ServiceBase
{
    Q_OBJECT
public:
    JamendoService( JamendoServiceFactory* parent, const QString &name );

    ~JamendoService();

    void polish();
    virtual Collections::Collection * collection() { return m_collection; }

private Q_SLOTS:
    void updateButtonClicked();
    void download();
    void listDownloadComplete( KJob* downloadJob);
    void listDownloadCancelled();
    void doneParsing();

    /**
    * Checks if download button should be enabled
    * @param selection the new selection
    */
    void itemSelected( CollectionTreeItem * selectedItem );


private:
    //DatabaseDrivenContentModel * m_model;
    QPushButton *m_updateListButton;
    QPushButton *m_downloadButton;
    KIO::FileCopyJob * m_listDownloadJob;
    QString m_tempFileName;
    Collections::ServiceSqlCollection * m_collection;
    Meta::JamendoAlbum * m_currentAlbum;

    JamendoXmlParser * m_xmlParser;
};

#endif

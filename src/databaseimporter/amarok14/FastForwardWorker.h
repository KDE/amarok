/*******************************************************************************
* copyright              : (C) 2008 Seb Ruiz <ruiz@kde.org>                    *
********************************************************************************/

/*******************************************************************************
*                                                                              *
*   This program is free software; you can redistribute it and/or modify       *
*   it under the terms of the GNU General Public License as published by       *
*   the Free Software Foundation; either version 2 of the License, or          *
*   (at your option) any later version.                                        *
*                                                                              *
********************************************************************************/


#ifndef AMAROK_FASTFORWARD_WORKER_H
#define AMAROK_FASTFORWARD_WORKER_H

#include "FastForwardImporter.h"
#include "databaseimporter/DatabaseImporter.h"
#include "meta/Meta.h"

#include <threadweaver/Job.h>
#include <threadweaver/ThreadWeaver.h>

#include <QSqlDatabase>

class FastForwardWorker : public ThreadWeaver::Job
{
    Q_OBJECT
    
    public:
        FastForwardWorker();

        void setDriver( const FastForwardImporter::ConnectionType &driver ) { m_driver = driver; }
        void setDatabaseLocation( const QString &location ) { m_databaseLocation = location; }
        void setDatabase( const QString &name ) { m_database = name; }
        void setHostname( const QString &host ) { m_hostname = host; }
        void setUsername( const QString &user ) { m_username = user; }
        void setPassword( const QString &pass ) { m_password = pass; }
        void setImportArtwork( const bool import ) { m_importArtwork = import; }

        bool failed() const { return m_failed; }
        void abort() { m_aborted = true; }

        virtual void run();

    signals:
        void importError( QString ); 
        void showMessage( QString );
        void trackAdded( Meta::TrackPtr );

    private:
        QSqlDatabase databaseConnection();
        const QString driverName() const;
        
        bool m_aborted;
        bool m_failed;

        FastForwardImporter::ConnectionType m_driver;
        QString m_databaseLocation;
        QString m_database;
        QString m_hostname;
        QString m_username;
        QString m_password;
        bool m_importArtwork;
};

#endif // multiple inclusion guard

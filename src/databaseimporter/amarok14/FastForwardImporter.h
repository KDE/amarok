/****************************************************************************************
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
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

#ifndef AMAROK_FASTFORWARD_IMPORTER_H
#define AMAROK_FASTFORWARD_IMPORTER_H

#include "databaseimporter/DatabaseImporter.h"

#include <QSqlDatabase>

class FastForwardImporterConfig;
class FastForwardWorker;

class FastForwardImporter : public DatabaseImporter
{
    Q_OBJECT

    public:
        FastForwardImporter( QObject *parent );
        virtual ~FastForwardImporter();

        virtual DatabaseImporterConfig *configWidget( QWidget *parent );
        
        static QString name() { return QString("amarok"); }

        virtual bool canImportArtwork() const { return true; }

        enum ConnectionType { SQLite=1, MySQL=2, PostgreSQL=3 };

    protected:
        virtual void import();

    private slots:
        void finishUp();

    private:
        FastForwardImporterConfig *m_config;
        FastForwardWorker         *m_worker;
};

#endif // multiple inclusion guard

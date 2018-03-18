/****************************************************************************************
 * Copyright (c) 2010 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef AMAROK_SQLBATCH_IMPORTER_H
#define AMAROK_SQLBATCH_IMPORTER_H

#include <QObject>

#include "core/meta/forward_declarations.h"

class SqlBatchImporterConfig;

/**
 * This importer will use the CollectionImportCapability to import a file.
 * Currently only used for the SqlCollection.
 */
class SqlBatchImporter : public QObject
{
    Q_OBJECT

    public:
        explicit SqlBatchImporter( QObject *parent );
        ~SqlBatchImporter();

        SqlBatchImporterConfig *configWidget( QWidget *parent );

        /**
         * @return whether the importer is running
         */
        bool importing() const;

        /**
         * Starts the importing process
         */
        void startImporting();

        /**
         * @returns the number of tracks imported
         */
        int importedCount() const;

    Q_SIGNALS:
        void importFailed();
        void importSucceeded();
        void importError( QString );
        void showMessage( QString );
        void trackAdded( Meta::TrackPtr );
        void trackDiscarded( QString );
        void trackMatchFound( Meta::TrackPtr, QString );
        void trackMatchMultiple( Meta::TrackList, QString );

    protected:
        void import();

    protected Q_SLOTS:
        void importingFinished();
        void trackImported( Meta::TrackPtr track );
        void trackMatched( Meta::TrackPtr track, QString oldUrl );

    private:
        SqlBatchImporterConfig *m_config;
        int m_count;
        bool m_importing;
};

#endif // multiple inclusion guard

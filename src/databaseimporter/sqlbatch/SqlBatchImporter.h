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

#include "databaseimporter/DatabaseImporter.h"

class SqlBatchImporterConfig;

/** This importer will use the CollectionImportCapability to import a file.
    Currently only used for the SqlCollection.
    */
class SqlBatchImporter : public DatabaseImporter
{
    Q_OBJECT

    public:
        SqlBatchImporter( QObject *parent );
        virtual ~SqlBatchImporter();

        virtual DatabaseImporterConfig *configWidget( QWidget *parent );

        static QString name() { return QString( "sql batch" ); }
    protected:
        virtual void import();

    private:
        SqlBatchImporterConfig *m_config;
};

#endif // multiple inclusion guard

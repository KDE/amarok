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


#ifndef AMAROK_ITUNES_IMPORTER_H
#define AMAROK_ITUNES_IMPORTER_H

#include "databaseimporter/DatabaseImporter.h"

class ITunesImporterConfig : public DatabaseImporterConfig
{
    public:
        ITunesImporterConfig( QWidget *parent = 0 );
        virtual ~ITunesImporterConfig() { };
};

class ITunesImporter : public DatabaseImporter
{
    Q_OBJECT

    public:
        ITunesImporter();
        virtual ~ITunesImporter();

        static QString name() { return QString("itunes"); }

        virtual bool canImportArtwork() const { return false; }

    protected:
        virtual void import();
};

#endif // multiple inclusion guard

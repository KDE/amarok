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


#ifndef AMAROK_DATABASE_IMPORTER_H
#define AMAROK_DATABASE_IMPORTER_H

#include <meta/Meta.h>

#include <KVBox>

class DatabaseImporter;

class DatabaseImporterFactory
{
    public:
        static DatabaseImporter* createImporter( const QString &name, QObject *parent );
};

class DatabaseImporterConfig : public KVBox
{
    public:
        DatabaseImporterConfig( QWidget *parent = 0 ) : KVBox( parent ) { }
        virtual ~DatabaseImporterConfig() { };
};

class DatabaseImporter : public QObject
{
    Q_OBJECT

    public:
        DatabaseImporter( QObject *parent=0 );
        virtual ~DatabaseImporter();

        virtual DatabaseImporterConfig *configWidget( QWidget *parent );

        /**
         * Determines whether this importer is able to retrieve artwork
         */
        virtual bool canImportArtwork() const = 0;

        /**
         * Sets the importer to import artwork when available
         */
        virtual void setImportArtwork( const bool importArtwork );

        /**
         * @return whether the importer will import artwork
         */
        virtual bool importArtwork() const;

        /**
         * @return whether the importer is running
         */
        virtual bool importing() const;

        /**
         * Starts the importing process
         */
        virtual void startImporting();

        /**
         * @returns the number of tracks imported
         */
        int importedCount() const;

    signals:
        void importFailed();
        void importSucceeded();
        void importError( QString );
        void trackAdded( Meta::TrackPtr );

    protected slots:
        void importingFinished();
        void trackImported( Meta::TrackPtr track );

    protected:
        /**
         * The internal implementation of the importing algorithm.
         */
        virtual void import() = 0;

        bool m_importArtwork;
        bool m_importing;

    private:
        int m_count;
};

#endif // multiple inclusion guard

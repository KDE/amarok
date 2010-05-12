/***************************************************************************
 *   Copyright (C) 2003-2010 Mark Kretschmann <kretschmann@kde.org>        *
 *             (C) 2008 Dan Meltzer <parallelgrapefruit@gmail.com>         *
 *             (C) 2008-2009 Jeff Mitchell <mitchell@kde.org>              *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef COLLECTIONSCANNER_H
#define COLLECTIONSCANNER_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDBusInterface>
#include <QHash>
#include <QSet>
#include <QStringList>

//Taglib includes..
#include <audioproperties.h>
#include <fileref.h>

typedef QHash<QString, QString> AttributeHash;

/**
 * @class CollectionScanner
 * @short Scans directories and builds the Collection
 */

class CollectionScanner : public QCoreApplication
{
    Q_OBJECT

public:
    CollectionScanner( int &argc, char **argv );

    ~CollectionScanner();
    int newInstance() { return 0; }

private slots:
    void doJob();

private:
    enum FileType
    {
        mp3,
        ogg,
        flac,
        mp4
    };

    bool readBatchIncrementalFile();
    bool readMtimeFile();

    void readDir( const QString& dir, QStringList& entries );
    void scanFiles( const QStringList& entries );
    
    /**
     * Read metadata tags of a given file.
     * @track Track for the file.
     * @return QMap containing tags, or empty QMap on failure.
     */
    AttributeHash readTags( const QString &path, TagLib::AudioProperties::ReadStyle readStyle = TagLib::AudioProperties::Fast );

    /**
     * Helper method for writing XML elements to stdout.
     * @name Name of the element.
     * @attributes Key/value map of attributes.
     */
    void writeElement( const QString& name, const AttributeHash& attributes );

    /**
     * @return the LOWERCASE file extension without the preceding '.', r "" if there is none
     */
    inline QString extension( const QString &fileName )
    {
        return fileName.contains( '.' ) ? fileName.mid( fileName.lastIndexOf( '.' ) + 1 ).toLower() : "";
    }

    /**
     * @return the last directory in @param fileName
     */
    inline QString directory( const QString &fileName )
    {
        return fileName.section( '/', 0, -2 );
    }

    QString escape( const QString &plain );
    void readArgs();
    void printVersionAndExit();
    void displayHelp();

    QString                     m_collectionId;
    QString                     m_amarokPid;
    bool                        m_batch;
    bool                        m_charset;
    bool                        m_importPlaylists;
    QStringList                 m_folders;
    QHash<QString, uint>        m_mTimeMap;
    QSet<QString>               m_scannedDirs;
    QDateTime                   m_batchFolderTime;
    bool                        m_recursively;
    bool                        m_incremental;
    bool                        m_restart;
    bool                        m_idlePriority;
    QString                     m_saveLocation;
    QString                     m_logfile;
    QString                     m_rpath;
    QString                     m_mtimeFile;
    QStringList                 m_scannedFolders;
    QDBusInterface             *m_amarokCollectionInterface;

    // Disable copy constructor and assignment
    CollectionScanner( const CollectionScanner& );
    CollectionScanner& operator= ( const CollectionScanner& );

};


#endif // COLLECTIONSCANNER_H


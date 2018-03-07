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

#include "collectionscanner/ScanningState.h"

#include <QCoreApplication>
#include <QHash>
#include <QSet>
#include <QStringList>
#include <QXmlStreamWriter>

namespace CollectionScanner
{

/**
 * @class Scanner
 * @short Scans directories and builds the Collection
 */
class Scanner : public QCoreApplication
{
    Q_OBJECT

public:
    Scanner( int &argc, char **argv );
    ~Scanner();

    /** Reads the batch file and adds the content to m_folders and m_Times */
    void readBatchFile( const QString &path );

    /** Get's the modified time from the given file and set's m_newerTime according */
    void readNewerTime( const QString &path );

private Q_SLOTS:
    void doJob();

private:
    void addDir( const QString& dir, QSet<QString> *entries );

    /** Returns true if the track is modified.
     *  Modification is determined first by m_mTimes and (if not found)
     *  then by m_newerTime
     */
    bool isModified( const QString& dir );

    void readArgs();

    /** Displays the error message and exits */
    void error( const QString &str );

    /** Displays the version and exits */
    void displayVersion();

    /** Displays the help and an optional error message and exits */
    void displayHelp( const QString &error = QString() );

    bool                  m_charset;
    QStringList           m_folders;

    uint                  m_newerTime;
    QHash<QString, uint>  m_mTimes;

    bool                  m_incremental;
    bool                  m_recursively;
    bool                  m_restart;
    bool                  m_idlePriority;

    QString               m_mtimeFile;
    ScanningState         m_scanningState;


    // Disable copy constructor and assignment
    Scanner( const Scanner& );
    Scanner& operator= ( const Scanner& );
};

}

#endif // COLLECTIONSCANNER_H

/****************************************************************************************
 * Copyright (C) 2003-2010 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (C) 2008 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
 * Copyright (C) 2008-2009 Jeff Mitchell <mitchell@kde.org>                             *
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#ifndef COLLECTIONSCANNER_SCANNINGSTATE_H
#define COLLECTIONSCANNER_SCANNINGSTATE_H

#include "amarokshared_export.h"

#include <QSharedMemory>
#include <QString>
#include <QStringList>

namespace CollectionScanner
{

/** A class used to store the current scanning state in a shared memory segment.
    Storing the state of the scanner shouldn't cause a file access.
    We are using a shared memory that the amarok process holds open until the scanning
    is finished to store the state.
 */
class AMAROKSHARED_EXPORT ScanningState
{
    public:
        ScanningState();
        ~ScanningState();

        void setKey( const QString &key );
        bool isValid() const;

        QString lastDirectory() const;
        void setLastDirectory( const QString &dir );

        QStringList badFiles() const;
        void setBadFiles( const QStringList &badFiles );

        QString lastFile() const;
        void setLastFile( const QString &file );

        void readFull();
        void writeFull();

    private:
        QSharedMemory *m_sharedMemory;

        QString m_lastDirectory;
        QStringList m_badFiles;
        QString m_lastFile;
        qint64 m_lastFilePos;
};

}

#endif // COLLECTIONSCANNER_SCANNINGSTATE_H

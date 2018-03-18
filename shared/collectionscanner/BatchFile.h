/***************************************************************************
 *   Copyright (C) 2010 Ralf Engels <ralf-engels@gmx.de>                   *
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

#ifndef COLLECTIONSCANNER_BATCHFILE_H
#define COLLECTIONSCANNER_BATCHFILE_H

#include "amarokshared_export.h"

#include <QString>
#include <QStringList>
#include <QPair>
#include <QList>

namespace CollectionScanner
{


/**
 * @class BatchFile
 * @short This class can be used to read and write batch files for the collection scanner.
 */

class AMAROKSHARED_EXPORT BatchFile
{
public:
    /** Constructs and empty BatchFile */
    BatchFile();

    /** Reads the BatchFile from the disk */
    explicit BatchFile( const QString &path );

    /** This type is a pair of directory path and modification time */
    typedef QPair<QString, uint> TimeDefinition;

    /** Those are the directories that should be added to the scanning list */
    const QStringList &directories() const;
    void setDirectories( const QStringList &value );

    const QList<TimeDefinition> &timeDefinitions() const;
    void setTimeDefinitions( const QList<TimeDefinition> &value );

    /** Writes the BatchFile to the disk.
        @returns true if writing was successful.
    */
    bool write( const QString &path );

private:
    QStringList m_directories;
    QList<TimeDefinition> m_timeDefinitions;
};

}
#endif // COLLECTIONSCANNER_BATCHFILE_H

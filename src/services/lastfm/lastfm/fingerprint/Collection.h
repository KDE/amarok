/***************************************************************************
 *   Copyright (C) 2007 by                                                 *
 *      Last.fm Ltd <client@last.fm>                                       *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

/*! \class Collection
    \brief Collection management class.
    super description, what does it store? what does it manage?
    bad name if it only handles fingerprints
*/

#ifndef COLLECTION_H
#define COLLECTION_H

#include "FingerprintDllExportMacro.h"
#include <QObject>
#include <QMutex>
#include <QSqlDatabase>


/** @author: <chris@last.fm> */
class FINGERPRINT_DLLEXPORT Collection : public QObject
{
    Q_OBJECT

public:
    /** \brief Returns the singleton instance to the controller. */
    static Collection&
    instance();

    /** \brief Terminates and deletes the collection instance. */
    void
    destroy();

    /** \brief Temp method: Gets a fingerprint id. Returns -1 if none found. */
    virtual QString
    getFingerprint( const QString& filePath );

    /** \brief Temp method: Sets a fingerprint id. */
    bool
    setFingerprint( const QString& filePath, QString fpId );

private:
    Collection();
    ~Collection();

    /** the database version
        * version 0: up until 1.4.1
        * version 1: from 1.4.2 */
    int
    version() const;

    virtual bool
    initDatabase();

    bool
    query( const QString& queryToken );

    QString
    fileURI( const QString& filePath );

    static Collection* s_instance;

    QMutex m_mutex;
    QSqlDatabase m_db;
    QString m_dbPath;
};

#endif // COLLECTION_H

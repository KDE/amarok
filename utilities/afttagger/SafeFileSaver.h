/*
 *  Copyright (c) 2008 Jeff Mitchell <kde-dev@emailgoeshere.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef SAFEFILESAVER_H
#define SAFEFILESAVER_H

#include <QString>
#include <QByteArray>

/**
 * @class SafeFileSaver
 * @author Jeff Mitchell <kde-dev@emailgoeshere.com>
 */

class SafeFileSaver
{
public:
     explicit SafeFileSaver( const QString &origPath );
    ~SafeFileSaver();

    QString prepareToSave();
    bool doSave();
    void failRemoveCopy( bool revert );
    bool cleanupSave();

    void setVerbose( bool verbose ) { m_verbose = verbose; }
    void setPrefix( const QString &prefix ) { if( !prefix.isEmpty() ) m_prefix = prefix; }

private:
    QString m_origPath;
    QString m_tempSavePath;
    QString m_origRenamedSavePath;
    QString m_tempSaveDigest;
    bool m_cleanupNeeded;
    bool m_verbose;
    QString m_prefix;
};

#endif


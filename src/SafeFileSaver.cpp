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

#define DEBUG_PREFIX "SafeFileSaver"

#include "Debug.h"
#include "SafeFileSaver.h"

#include <KRandom>
#include <KMD5>

#include <QFile>
#include <QString>

#ifdef Q_WS_WIN
#include <process.h>
#else
#include <unistd.h>
#include <sys/types.h>
#endif

SafeFileSaver::SafeFileSaver( const QString &origPath )
    : m_origPath( origPath )
    , m_tempSavePath()
    , m_origRenamedSavePath()
    , m_tempSaveDigest( 0 )
    , m_cleanupNeeded( false )

{
    DEBUG_BLOCK
}

SafeFileSaver::~SafeFileSaver()
{
    DEBUG_BLOCK
    if( m_cleanupNeeded )
        cleanupSave();
}

QString
SafeFileSaver::prepareToSave()
{
    DEBUG_BLOCK

    m_cleanupNeeded = true;
    KMD5 md5sum;

    QString pid;
#ifdef Q_WS_WIN
    pid.setNum( _getpid() );
#else
    pid.setNum( getpid() );
#endif

    QString randomString = KRandom::randomString( 8 );

    m_tempSavePath = m_origPath + ".amaroktemp.pid-" + pid + ".random-" + randomString;
    m_origRenamedSavePath = m_origPath + ".amarokoriginal.pid-" + pid + ".random-" + randomString;


    debug() << "Copying original file to copy and caluclating MD5" << endl;

    if( !QFile::copy( m_origPath, m_tempSavePath ) )
    {
        debug() << "Could not copy the file.  Check that you have sufficient permissions/disk space "
                   "and that the destination path does not already exist!" << endl;
        return QString();
    }

    QFile tempFile( m_tempSavePath );
    if( !tempFile.open( QIODevice::ReadOnly | QIODevice::Unbuffered ) )
    {
        debug() << "Could not open temp file for MD5 calculation!" << endl;
        return QString();
    }

    if( !md5sum.update( tempFile ) )
    {
        debug() << "Could not get checksum of temp file!" << endl;
        tempFile.close();
        return QString();
    }
    
    m_tempSaveDigest = md5sum.hexDigest();

    tempFile.close();

    //By this point, we have the following:
    //The original file is copied at path m_tempSavePath
    //We have generated what will be the filename to rename the original to in m_origRenamedSavePath
    //We have successfully copied the original file to the temp location
    //We've calculated the md5sum of the original file

    debug() << "MD5 sum of temp file: " << m_tempSaveDigest.data() << endl;

    //Now, we have a MD5 sum of the original file at the time of copying saved in m_tempSaveDigest

    return m_tempSavePath;
}

bool
SafeFileSaver::doSave()
{
    //TODO: much commenting needed.  For now this pretty much follows algorithm laid out in bug 131353,
    //but isn't useable since I need to find a good way to switch the file path with taglib, or a good way
    //to get all the metadata copied over.

    DEBUG_BLOCK
    m_cleanupNeeded = true;

    KMD5 md5sum;

    QByteArray origRenamedDigest;

    if( m_tempSavePath.isEmpty() || m_tempSaveDigest.isEmpty() || m_origRenamedSavePath.isEmpty() )
    {
        debug() << "You must run prepareToSave() and it must return successfully before calling doSave()!" << endl;
        return false;
    }

    debug() << "Renaming original file to temporary name " << m_origRenamedSavePath << endl;

    if( !QFile::rename( m_origPath, m_origRenamedSavePath ) )
    {
        debug() << "Could not move original!" << endl;
        failRemoveCopy( false );
        return false;
    }

    debug() << "Calculating MD5 of " << m_origRenamedSavePath << endl;

    QFile origRenamedFile( m_origRenamedSavePath );
    if( !origRenamedFile.open( QIODevice::ReadOnly | QIODevice::Unbuffered ) )
    {
        debug() << "Could not open temporary file!" << endl;
        failRemoveCopy( true );
        return false;
    }

    
    if( !md5sum.update( origRenamedFile ) )
    {
        debug() << "Error during checksumming temp file!" << endl;
        origRenamedFile.close();
        failRemoveCopy( true );
        return false;
    }

    origRenamedDigest = md5sum.hexDigest();

    origRenamedFile.close();

    debug() << "md5sum of original renamed file: " << origRenamedDigest.data() << endl;

    if( origRenamedDigest != m_tempSaveDigest )
    {
        debug() << "Original checksum did not match current checksum!" << endl;
        failRemoveCopy( true );
        return false;
    }

    debug() << "Renaming temp file to original's filename" << endl;

    if( !QFile::rename( m_tempSavePath, m_origPath ) )
    {
        debug() << "Could not rename new file to original!" << endl;
        failRemoveCopy( true );
        return false;
    }

    debug() << "Deleting original" << endl;

    if( !QFile::remove( m_origRenamedSavePath ) )
    {
        debug() << "Could not delete the original file!" << endl;
        return false;
    }

    debug() << "Save done, returning true!" << endl;

    return true;
}

void
SafeFileSaver::failRemoveCopy( bool revert )
{
    debug() << "Deleting temporary file..." << endl;
    if( !QFile::remove( m_tempSavePath ) )
        debug() << "Could not delete the temporary file!" << endl;

    if( !revert )
        return;

    debug() << "Reverting original file to original filename!" << endl;
    if( !QFile::rename( m_origRenamedSavePath, m_origPath ) )
        debug() << "Could not revert file to original filename!" << endl;

}

bool
SafeFileSaver::cleanupSave()
{
    DEBUG_BLOCK

    bool dirty = false;

    if( !m_tempSavePath.isEmpty() && QFile::exists( m_tempSavePath ) )
    {
        if( !QFile::remove( m_tempSavePath ) )
        {
            dirty = true;
            debug() << "Could not delete the temporary file!" << endl;
        }
    }

    m_tempSavePath.clear();
    m_origRenamedSavePath.clear();
    m_tempSaveDigest.clear();

    m_cleanupNeeded = false;
    return !dirty;
}


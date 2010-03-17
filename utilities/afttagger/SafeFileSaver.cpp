/*
 *  Copyright (c) 2008-2009 Jeff Mitchell <mitchell@kde.org>
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

#include "SafeFileSaver.h"

#include <QtDebug>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
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
    , m_cleanupNeeded( false )
    , m_verbose( false )
    , m_prefix( "safefilesaver" )
{
}

SafeFileSaver::~SafeFileSaver()
{
    if( m_cleanupNeeded )
        cleanupSave();
}

QString
SafeFileSaver::prepareToSave()
{
    if( m_verbose )
        qDebug() << "prepareToSave start";
    m_cleanupNeeded = true;
    QCryptographicHash md5sum( QCryptographicHash::Md5 ); 

    QString pid;
#ifdef Q_WS_WIN
    pid.setNum( _getpid() );
#else
    pid.setNum( getpid() );
#endif

    int length = 8;
    //The following snippet of code is copied from kdelibs, and is copyright Matthias Kalle Dalheimer, Charles Samuels, Joseph Wenninger, and maybe more
    QString str; str.resize( length );
    int i = 0;
    while( length-- )
    {
        int r = qrand() % 62;
        r+=48;
        if( r > 57 ) r+=7;
        if( r > 90 ) r+=6;
        str[i++] = char( r );
    }

    QString randomString = str;

    m_tempSavePath = m_origPath + '.' + m_prefix + "temp.pid-" + pid + ".random-" + randomString + '.' + QFileInfo( m_origPath ).suffix();
    m_origRenamedSavePath = m_origPath + '.' + m_prefix + "original.pid-" + pid + ".random-" + randomString + '.' + QFileInfo( m_origPath ).suffix();


    if( m_verbose )
        qDebug() << "Copying original file " << m_origPath << " to copy at " << m_tempSavePath << " and caluclating MD5";

    if( !QFile::copy( m_origPath, m_tempSavePath ) )
    {
        if( m_verbose )
            qDebug() << "Could not copy the file.  Check that you have sufficient permissions/disk space "
                   "and that the destination path does not already exist!";
        return QString();
    }

    QFile tempFile( m_tempSavePath );
    if( !tempFile.open( QIODevice::ReadOnly | QIODevice::Unbuffered ) )
    {
        if( m_verbose )
            qDebug() << "Could not open temp file for MD5 calculation!";
        return QString();
    }

    md5sum.addData( tempFile.readLine() );
    m_tempSaveDigest = md5sum.result().toHex();

    tempFile.close();

    //By this point, we have the following:
    //The original file is copied at path m_tempSavePath
    //We have generated what will be the filename to rename the original to in m_origRenamedSavePath
    //We have successfully copied the original file to the temp location
    //We've calculated the md5sum of the original file

    if( m_verbose )
        qDebug() << "MD5 sum of temp file: " << m_tempSaveDigest;

    //Now, we have a MD5 sum of the original file at the time of copying saved in m_tempSaveDigest

    return m_tempSavePath;
}

bool
SafeFileSaver::doSave()
{
    if( m_verbose )
        qDebug() << "doSave start";
    //TODO: much commenting needed.  For now this pretty much follows algorithm laid out in bug 131353,
    //but isn't useable since I need to find a good way to switch the file path with taglib, or a good way
    //to get all the metadata copied over.

    m_cleanupNeeded = true;

    QCryptographicHash md5sum( QCryptographicHash::Md5 ); 

    QString origRenamedDigest;

    if( m_tempSavePath.isEmpty() || m_tempSaveDigest.isEmpty() || m_origRenamedSavePath.isEmpty() )
    {
        if( m_verbose)
            qDebug() << "You must run prepareToSave() and it must return successfully before calling doSave()!";
        return false;
    }

    if( m_verbose )
        qDebug() << "Renaming original file to temporary name " << m_origRenamedSavePath;

    if( !QFile::rename( m_origPath, m_origRenamedSavePath ) )
    {
        if( m_verbose )
            qDebug() << "Could not move original!";
        failRemoveCopy( false );
        return false;
    }

    if( m_verbose )
        qDebug() << "Calculating MD5 of " << m_origRenamedSavePath;

    QFile origRenamedFile( m_origRenamedSavePath );
    if( !origRenamedFile.open( QIODevice::ReadOnly | QIODevice::Unbuffered ) )
    {
        if( m_verbose )
            qDebug() << "Could not open temporary file!";
        failRemoveCopy( true );
        return false;
    }

    md5sum.addData( origRenamedFile.readLine() );
    origRenamedDigest = md5sum.result().toHex();
    origRenamedFile.close();

    if( m_verbose )
        qDebug() << "md5sum of original renamed file: " << origRenamedDigest;

    if( origRenamedDigest != m_tempSaveDigest )
    {
        if( m_verbose )
            qDebug() << "Original checksum did not match current checksum!";
        failRemoveCopy( true );
        return false;
    }

    if( m_verbose )
        qDebug() << "Renaming temp file to original's filename";

    if( !QFile::rename( m_tempSavePath, m_origPath ) )
    {
        if( m_verbose )
            qDebug() << "Could not rename new file to original!";
        failRemoveCopy( true );
        return false;
    }

    if( m_verbose )
        qDebug() << "Deleting original";

    if( !QFile::remove( m_origRenamedSavePath ) )
    {
        if( m_verbose )
            qDebug() << "Could not delete the original file!";
        return false;
    }

    if( m_verbose )
        qDebug() << "Save done, returning true!";

    return true;
}

void
SafeFileSaver::failRemoveCopy( bool revert )
{
    if( m_verbose )
        qDebug() << "failRemoveCopy start";
    if( !QFile::remove( m_tempSavePath ) )
    {
        if( m_verbose )
            qDebug() << "Could not delete the temporary file!";
    }

    if( !revert )
        return;

    if( m_verbose )
        qDebug() << "Reverting original file to original filename!";
    if( !QFile::rename( m_origRenamedSavePath, m_origPath ) )
    {
        if( m_verbose )
            qDebug() << "Could not revert file to original filename!";
    }
}

bool
SafeFileSaver::cleanupSave()
{
    if( m_verbose )
        qDebug() << "cleanupSave start";
    bool dirty = false;

    if( !m_tempSavePath.isEmpty() && QFile::exists( m_tempSavePath ) )
    {
        if( !QFile::remove( m_tempSavePath ) )
        {
            dirty = true;
            if( m_verbose )
                qDebug() << "Could not delete the temporary file!";
        }
    }

    m_tempSavePath.clear();
    m_origRenamedSavePath.clear();
    m_tempSaveDigest.clear();

    m_cleanupNeeded = false;
    return !dirty;
}


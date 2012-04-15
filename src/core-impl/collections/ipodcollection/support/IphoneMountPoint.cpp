/****************************************************************************************
 * Copyright (c) 2010 Martin Aumueller <aumuell@reserv.at>                              *
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

#include "IphoneMountPoint.h"

#include "core/support/Debug.h"

#include <KStandardDirs>

#include <QProcess>
#include <QDir>


IphoneMountPoint::IphoneMountPoint( const QString &uuid )
{
    QStringList args;
    if( !uuid.isEmpty() )
    {
        args << "--uuid";
        args << uuid;
        args << QString("-ofsname=afc://%1").arg( uuid );
    }
    QString mountPointCandidate = constructMountpoint( uuid );
    args << mountPointCandidate;

    // TODO: refactor this so that it is not that horrible
    QProcess ifuse;
    debug() << "calling ifuse with args" << args;
    ifuse.start( "ifuse", args);
    bool ok = ifuse.waitForStarted( 5000 );
    if( !ok )
        debug() << "Failed to start ifuse";
    else
    {
        ok = ifuse.waitForFinished();
        if( !ok )
            debug() << "ifuse did not yet terminate";
    }
    if( ok )
    {
        ok = ifuse.exitStatus() == QProcess::NormalExit;
        if( !ok )
            debug() << "ifuse crashed";
    }
    if( ok )
    {
        ok = ifuse.exitCode() == 0;
        if( !ok )
        {
            if( ifuse.exitCode() == 255 )
            {
                debug() << "ipod mount dir was not cleanly unmounted, attempting unmount";
                QProcess unmount;
                QStringList unmountarg;
                unmountarg << "-u" << mountPointCandidate;
                unmount.start("fusermount", unmountarg);
                bool unmountok = unmount.waitForStarted();
                if( !unmountok )
                {
                    debug() << "fusermount for unmounting" << mountPointCandidate << "failed to start";
                }
                else
                {
                    unmountok = unmount.waitForFinished();
                    if( !unmountok )
                        debug() << "fusermount did not terminate correctly";
                }
                if( unmountok )
                {
                    unmountok = unmount.exitStatus() == QProcess::NormalExit;
                    if( !unmountok )
                        debug() << "fusermount did not exit normally";
                    else
                    {
                        // take 2
                        debug() << "calling ifuse with args" << args;
                        ifuse.start("ifuse", args);
                        ok = ifuse.waitForStarted();
                        if( !ok )
                        {
                            debug() << "Failed to start ifuse";
                        }
                        else
                        {
                            ok = ifuse.waitForFinished();
                            if( !ok )
                                debug() << "ifuse did not yet terminate";
                        }
                        if( ok )
                        {
                            ok = ifuse.exitStatus() == QProcess::NormalExit;
                            if( !ok )
                                debug() << "ifuse crashed";
                        }
                        if( ok )
                        {
                            ok = ifuse.exitCode() == 0;
                            if( !ok )
                            {
                                debug() << "ifuse exited with non-zero exit code" << ifuse.exitCode();
                            }
                        }
                    }
                }
            }
            else
                debug() << "ifuse exited with non-zero exit code" << ifuse.exitCode();
        }
    }

    if( ok )
    {
        m_mountPoint = mountPointCandidate;
        debug() << "Successfully mounted imobiledevice using ifuse on" << mountPointCandidate;
    }
    else
    {
        debug() << "Mounting imobiledevice using ifuse on" << mountPointCandidate << "failed";
    }
}

IphoneMountPoint::~IphoneMountPoint()
{
    if( mountPoint().isEmpty() )
        return; // easy, nothing to do

    QProcess unmount;
    QStringList args;
    args << "-u" << "-z" << mountPoint();
    unmount.start("fusermount", args);
    bool ok = unmount.waitForStarted();
    if( !ok )
        debug() << "fusermount for unmounting" << mountPoint() << "failed to start";
    else
    {
        ok = unmount.waitForFinished();
        if( !ok )
            debug() << "fusermount did not terminate correctly";
    }

    if( ok )
    {
        ok = unmount.exitStatus() == QProcess::NormalExit;
        if( !ok )
            debug() << "fusermount did not exit normally";
    }

    if( ok )
    {
        ok = unmount.exitCode() == 0;
        if( !ok )
            debug() << "fusermount did not exit successfully";
    }

    if( ok )
        debug() << "Unmounted imobiledevice using ifuse from" << mountPoint();
    else
        debug() << "Unmounting imobiledevice using ifuse from" << mountPoint() << "failed";

    if( ok )
    {
        if( QDir( mountPoint() ).rmpath( "." ) )
            debug() << "Deleted" << mountPoint() << "directory and empty parent directories";
        else
            debug() << "Failed to delete" << mountPoint() << "directory.";
    }
}

QString
IphoneMountPoint::mountPoint() const
{
    return m_mountPoint;
}

QString
IphoneMountPoint::constructMountpoint( const QString &uuid )
{
    QString mountPointCandidate = KStandardDirs::locateLocal( "tmp", "amarok/" );
    mountPointCandidate += "imobiledevice";
    if( !uuid.isEmpty() )
        mountPointCandidate += '_' + uuid;
    debug() << "set mountpoint to " << mountPointCandidate;

    QDir mp( mountPointCandidate );
    if( !mp.exists() )
    {
        mp.mkpath( mountPointCandidate );
        debug() << "created " << mountPointCandidate;
    }
    return mountPointCandidate;
}

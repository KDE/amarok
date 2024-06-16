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

#include <QStandardPaths>
#include <KLocalizedString>
#include <KMessageBox>

#include <QProcess>
#include <QDir>


IphoneMountPoint::IphoneMountPoint( const QString &uuid )
{
    QString mountPointCandidate = constructMountpoint( uuid );
    QStringList checkedDirs;  // see itdb_get_control_dir (const gchar *mountpoint)
    checkedDirs << "/iTunes_Control";
    checkedDirs << "/iPod_Control";
    checkedDirs << "/iTunes/iTunes_Control";
    for( const QString &dir : checkedDirs )
    {
        if( QFile::exists( mountPointCandidate + dir ) )
        {
            logMessage( QString( "%1 exists, assuming iPhone is already mounted" ).arg( dir ) );
            m_mountPoint = mountPointCandidate;
            return;
        }
    }

    QStringList args;
    if( !uuid.isEmpty() )
        // good change here: --uuid option was renamed to --udid with ifuse-1.1.2, the
        // short option, -u, fortunately remained the same
        args << "-u" << uuid << QString( "-ofsname=afc://%1" ).arg( uuid );
    args << mountPointCandidate;
    if( !call( "ifuse", args ) )
    {
        logMessage( QString( "Failed to mount iPhone on %1" ).arg( mountPointCandidate ) );
        KMessageBox::detailedError( nullptr, i18n( "Connecting to iPhone, iPad or iPod touch failed."),
            failureDetails() );
        return;
    }
    logMessage( QString( "Successfully mounted iPhone on %1" ).arg( mountPointCandidate ) );
    m_mountPoint = mountPointCandidate;

}

IphoneMountPoint::~IphoneMountPoint()
{
    if( m_mountPoint.isEmpty() )
        return; // easy, nothing to do

    logMessage( "" );  // have a line between constructor and destructor messages

    if( !call( "fusermount", QStringList() << "-u" << "-z" << m_mountPoint ) )
    {
        logMessage( QString( "Failed to unmount iPhone from %1" ).arg( m_mountPoint ) );
        return;
    }
    logMessage( QString( "Successfully unmounted iPhone from %1" ).arg( m_mountPoint ) );

    if( QDir( mountPoint() ).rmpath( "." ) )
        logMessage( QString( "Deleted %1 directory and empty parent directories" ).arg( m_mountPoint ) );
    else
        logMessage( QString( "Failed to delete %1 directory" ).arg( m_mountPoint ) );
}

QString
IphoneMountPoint::mountPoint() const
{
    return m_mountPoint;
}

QString IphoneMountPoint::failureDetails() const
{
    return m_messages.join( "<br>\n" );
}

QString
IphoneMountPoint::constructMountpoint( const QString &uuid )
{
    QString mountPointCandidate = QStandardPaths::locate( QStandardPaths::TempLocation, "amarok/" );
    mountPointCandidate += "imobiledevice";
    if( !uuid.isEmpty() )
        mountPointCandidate += "_uuid_" + uuid;
    logMessage( QStringLiteral( "determined mount-point path to %1" ).arg( mountPointCandidate ) );

    QDir mp( mountPointCandidate );
    if( !mp.exists() )
    {
        mp.mkpath( mountPointCandidate );
        logMessage( QStringLiteral( "created %1 directory" ).arg( mountPointCandidate ) );
    }
    return mountPointCandidate;
}

bool IphoneMountPoint::call( const QString &command, const QStringList &arguments, int timeout )
{
    QProcess process;
    process.setProcessChannelMode( QProcess::MergedChannels );
    logMessage( QStringLiteral( "calling `%1 \"%2\"` with timeout of %3s" ).arg( command, arguments.join( "\" \"" ) ).arg( timeout/1000.0 ) );
    process.start( command, arguments );

    if( !process.waitForStarted( timeout ) )
    {
        logMessage( "command failed to start within timeout" );
        return false;
    }
    if( !process.waitForFinished( timeout ) )
    {
        logMessage( "command failed to finish within timeout" );
        return false;
    }

    QByteArray output( process.readAllStandardOutput() );
    for( const QString &line : QString::fromLocal8Bit( output ).split( QChar( '\n' ) ) )
    {
        logMessage( QStringLiteral("%1: %2").arg( command, line ) );
    }

    if( process.exitStatus() != QProcess::NormalExit )
    {
        logMessage( "command crashed" );
        return false;
    }
    if( process.exitCode() != 0 )
    {
        logMessage( QString( "command exited with non-zero return code %1" ).arg( process.exitCode() ) );
        return false;
    }
    return true;
}

void IphoneMountPoint::logMessage( const QString &message )
{
    m_messages << message;
    if( !message.isEmpty() )
        debug() << "IpodCollection: IphoneMountPoint:" << message.toLocal8Bit().constData();
}

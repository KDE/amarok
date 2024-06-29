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
    checkedDirs << QStringLiteral("/iTunes_Control");
    checkedDirs << QStringLiteral("/iPod_Control");
    checkedDirs << QStringLiteral("/iTunes/iTunes_Control");
    for( const QString &dir : checkedDirs )
    {
        if( QFile::exists( mountPointCandidate + dir ) )
        {
            logMessage( QStringLiteral( "%1 exists, assuming iPhone is already mounted" ).arg( dir ) );
            m_mountPoint = mountPointCandidate;
            return;
        }
    }

    QStringList args;
    if( !uuid.isEmpty() )
        // good change here: --uuid option was renamed to --udid with ifuse-1.1.2, the
        // short option, -u, fortunately remained the same
        args << QStringLiteral("-u") << uuid << QStringLiteral( "-ofsname=afc://%1" ).arg( uuid );
    args << mountPointCandidate;
    if( !call( QStringLiteral("ifuse"), args ) )
    {
        logMessage( QStringLiteral( "Failed to mount iPhone on %1" ).arg( mountPointCandidate ) );
        KMessageBox::detailedError( nullptr, i18n( "Connecting to iPhone, iPad or iPod touch failed."),
            failureDetails() );
        return;
    }
    logMessage( QStringLiteral( "Successfully mounted iPhone on %1" ).arg( mountPointCandidate ) );
    m_mountPoint = mountPointCandidate;

}

IphoneMountPoint::~IphoneMountPoint()
{
    if( m_mountPoint.isEmpty() )
        return; // easy, nothing to do

    logMessage( QStringLiteral("") );  // have a line between constructor and destructor messages

    if( !call( QStringLiteral("fusermount"), QStringList() << QStringLiteral("-u") << QStringLiteral("-z") << m_mountPoint ) )
    {
        logMessage( QStringLiteral( "Failed to unmount iPhone from %1" ).arg( m_mountPoint ) );
        return;
    }
    logMessage( QStringLiteral( "Successfully unmounted iPhone from %1" ).arg( m_mountPoint ) );

    if( QDir( mountPoint() ).rmpath( QStringLiteral(".") ) )
        logMessage( QStringLiteral( "Deleted %1 directory and empty parent directories" ).arg( m_mountPoint ) );
    else
        logMessage( QStringLiteral( "Failed to delete %1 directory" ).arg( m_mountPoint ) );
}

QString
IphoneMountPoint::mountPoint() const
{
    return m_mountPoint;
}

QString IphoneMountPoint::failureDetails() const
{
    return m_messages.join( QStringLiteral("<br>\n") );
}

QString
IphoneMountPoint::constructMountpoint( const QString &uuid )
{
    QString mountPointCandidate = QStandardPaths::locate( QStandardPaths::TempLocation, QStringLiteral("amarok/") );
    mountPointCandidate += QStringLiteral("imobiledevice");
    if( !uuid.isEmpty() )
        mountPointCandidate += QStringLiteral("_uuid_") + uuid;
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
    logMessage( QStringLiteral( "calling `%1 \"%2\"` with timeout of %3s" ).arg( command, arguments.join( QStringLiteral("\" \"") ) ).arg( timeout/1000.0 ) );
    process.start( command, arguments );

    if( !process.waitForStarted( timeout ) )
    {
        logMessage( QStringLiteral("command failed to start within timeout") );
        return false;
    }
    if( !process.waitForFinished( timeout ) )
    {
        logMessage( QStringLiteral("command failed to finish within timeout") );
        return false;
    }

    QByteArray output( process.readAllStandardOutput() );
    for( const QString &line : QString::fromLocal8Bit( output ).split( QLatin1Char( '\n' ) ) )
    {
        logMessage( QStringLiteral("%1: %2").arg( command, line ) );
    }

    if( process.exitStatus() != QProcess::NormalExit )
    {
        logMessage( QStringLiteral("command crashed") );
        return false;
    }
    if( process.exitCode() != 0 )
    {
        logMessage( QStringLiteral( "command exited with non-zero return code %1" ).arg( process.exitCode() ) );
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

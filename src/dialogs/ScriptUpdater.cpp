/****************************************************************************************
 * Copyright (c) 2009 Jakob Kummerow <jakob.kummerow@gmail.com>                         *
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

#define DEBUG_PREFIX "ScriptUpdater"

#include "ScriptUpdater.h"
#include "amarokconfig.h"

#include "Debug.h"

#include <KIO/Job>
#include <KIO/NetAccess>
#include <KStandardDirs>
#include <KTar>

#include <QTimer>
#include <QFileInfo>

#include <QtCrypto>

ScriptUpdater::ScriptUpdater() : QThread()
{
    // configuration
    m_updateBaseUrl     = "http://home.in.tum.de/~kummeroj/update/"; // must end with '/'
    m_archiveFilename   = "/main.tar.gz";  // must start with '/'
    m_versionFilename   = "/version";      // must start with '/'
    m_signatureFilename = "/signature";    // must start with '/'
    // TODO: remember to change the public key before release!
    m_publicKey = "-----BEGIN PUBLIC KEY-----"
"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAqSVx2dsSkfNniS/bK81q"
"JqyWsBiOaTFcvKn3SsQ8hWlPiyYgJUc0BFThbpOLw0et2cxvgCCryudWigCW5iNq"
"DeOYU2rC+fWjqMJMV/pSMQKIDtvlZRKpR6pmqcWSlpfLXxTVHPKBk4LKcb62O4Vi"
"TUQ6YYDQuMeDmpvdNJLRJtHs3ZAT5nLxLGP5TqLgcBtnte43uNgdJ1FSDROSwQcS"
"JpwhhEWsMnHB8wC6kr2oS721DJscMdGkqPvDZqqUqCfybzyFy20kFZ6ws5Ae4LgQ"
"c6vqkUUfaeiFx2Cx2htEgU4A1tze58h7Om3q3YXX1Rpl+iEMCMLAKRVHwYkLkUSe"
"sQIDAQAB"
"-----END PUBLIC KEY-----";
}

ScriptUpdater::~ScriptUpdater()
{
}

void
ScriptUpdater::setScriptPath( const QString& scriptPath )
{
    m_scriptPath = scriptPath;
}

void
ScriptUpdater::run()
{
    // only start the thread's event loop here. Everything else will be controlled via signals.
    this->exec();
    debug() << m_scriptname << ": Event loop terminated, quitting.";
    // once this ScriptUpdater is done, emit the corresponding signal
    emit finished( m_scriptPath );
}

void
ScriptUpdater::updateScript()
{
    DEBUG_BLOCK

    // cancel immediately if auto updating is disabled
    if( !AmarokConfig::autoUpdateScripts() )
    {
        QTimer::singleShot( 0, this, SLOT( quit() ) );
        return;
    }

    // 1. detect script name, open main.js and read version information
    QFile file(m_scriptPath);
    m_fileName = file.fileName();
    QRegExp rxname("amarok/scripts/(.+)/main.js");
    rxname.indexIn(m_fileName);
    m_scriptname = rxname.cap(1);
    QRegExp rxver("AmarokScriptVersion\\s*=\\s*(\\d+)\\b");
    file.open(QIODevice::ReadOnly);
    QString line(file.readLine());
    file.close();
    if (rxver.indexIn(line) != -1)
    {
        m_scriptversion = rxver.cap(1);
    } else {
        // if no version information was found, cancel the update
        QTimer::singleShot( 0, this, SLOT( quit() ) );
        return;
    }

    // 2. check if there are updates: get 'update' file from server
    KUrl versionUrl( m_updateBaseUrl );
    versionUrl.addPath( m_scriptname );
    versionUrl.addPath( m_versionFilename );
    debug() << m_scriptname << ": Accessing " << versionUrl.prettyUrl() << " ...";
    KJob* kjob = KIO::storedGet( versionUrl , KIO::NoReload, KIO::HideProgressInfo );
    connect ( kjob, SIGNAL( result( KJob* ) ), this, SLOT( phase2( KJob* )) );
}

void
ScriptUpdater::phase2( KJob * job )
{
    if ( job->error() )
    {
        // if no 'version' file was found, cancel the update
        QTimer::singleShot( 0, this, SLOT( quit() ) );
        return;
    }
    QString response = QString( ((KIO::StoredTransferJob*) job)->data() );
    if ( response.toInt() <= m_scriptversion.toInt() )
    {
        // if no newer version is available, cancel update
        QTimer::singleShot( 0, this, SLOT( quit() ) );
        return;
    }
    debug() << m_scriptname << ": newer version found, starting update :-)";

    // 3. get the update archive, download it to a temporary file
    KUrl archiveSrc( m_updateBaseUrl );
    archiveSrc.addPath( m_scriptname );
    archiveSrc.addPath( m_archiveFilename );
    m_archiveFile.open(); // temporary files only have a fileName() after they've been opened
    KUrl archiveDest(m_archiveFile.fileName());
    KIO::FileCopyJob *archiveJob = KIO::file_copy( archiveSrc, archiveDest, -1, KIO::Overwrite | KIO::HideProgressInfo );
    connect ( archiveJob, SIGNAL( result( KJob* ) ), this, SLOT( phase3( KJob* )) );
}

void ScriptUpdater::phase3( KJob * job )
{
    if ( job->error() )
    {
        // if the file wasn't found, cancel the update
        QTimer::singleShot( 0, this, SLOT( quit() ) );
        return;
    }

    // 4. get the archive's signature, download it to a temporary file as well
    KUrl sigSrc( m_updateBaseUrl );
    sigSrc.addPath( m_scriptname );
    sigSrc.addPath( m_signatureFilename );
    m_sigFile.open();
    KUrl sigDest( m_sigFile.fileName() );
    KIO::FileCopyJob *sigJob = KIO::file_copy( sigSrc, sigDest, -1, KIO::Overwrite | KIO::HideProgressInfo );
    connect ( sigJob, SIGNAL( result( KJob* ) ), this, SLOT( phase4( KJob* )) );
}

void
ScriptUpdater::phase4( KJob * job )
{
    if ( job->error() )
    {
        // if the signature couldn't be downloaded, cancel the update
        QTimer::singleShot( 0, this, SLOT( quit() ) );
        return;
    }

    // 5. compare the signature to the archive's hash
    QCA::Initializer init;
    QCA::ConvertResult conversionResult;
    QCA::PublicKey pubkey = QCA::PublicKey::fromPEM( m_publicKey, &conversionResult );
    if ( !( QCA::ConvertGood == conversionResult ) )
    {
        debug() << m_scriptname << "Failed to read public key!";
        QTimer::singleShot( 0, this, SLOT( quit() ) );
        return;
    }
    QFile file( m_archiveFile.fileName() );
    if ( !file.open( QIODevice::ReadOnly ) ) {
        debug() << m_scriptname << "Failed to open file for reading!";
        QTimer::singleShot( 0, this, SLOT( quit() ) );
        return;
    }
    QCA::Hash hash( "sha1" );
    hash.update( &file );
    file.close();
    QFile sigFile( m_sigFile.fileName() );
    if ( !sigFile.open( QIODevice::ReadOnly ) )
    {
        debug() << m_scriptname << "Failed to open signature file for reading!";
        QTimer::singleShot( 0, this, SLOT( quit() ) );
        return;
    }
    QByteArray signature = QByteArray::fromBase64( sigFile.readAll() );
    sigFile.close();
    pubkey.startVerify( QCA::EMSA3_SHA1 );
    pubkey.update( hash.final() );
    if ( !pubkey.validSignature( signature ) )
    {
        debug() << m_scriptname << "Invalid signature, no update performed.";
        QTimer::singleShot( 0, this, SLOT( quit() ) );
        return;
    }
    debug() << m_scriptname << ": Signature matches. Performing update now.";

    // 6. everything OK, perform the update by extracting the archive
    KTar archive( m_archiveFile.fileName() );
    if( !archive.open( QIODevice::ReadOnly ) )
    {
        // in case of errors: bad luck, cancel the update
        debug() << m_scriptname << ": Error opening the update package.";
        QTimer::singleShot( 0, this, SLOT( quit() ) );
        return;
    }
    QString relPath = KGlobal::dirs()->relativeLocation( "data", m_fileName );
    QFileInfo fileinfo( relPath );
    QString destination = KGlobal::dirs()->saveLocation( "data", fileinfo.path(), false );
    const KArchiveDirectory* const archiveDir = archive.directory();
    archiveDir->copyTo( destination );
    debug() << m_scriptname << ": Updating finished successfully :-)";

    // all done, temporary files are deleted automatically by Qt
    QTimer::singleShot( 0, this, SLOT( quit() ) );
}


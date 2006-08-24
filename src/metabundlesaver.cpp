// Jeff Mitchell <kde-dev@emailgoeshere.com>, (C) 2006
// License: GNU General Public License V2


#define DEBUG_PREFIX "MetaBundleSaverSaver"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>

#include "amarok.h"
#include "amarokconfig.h"
#include "collectiondb.h"
#include "debug.h"
#include "metabundlesaver.h"
#include "scancontroller.h"
#include "statusbar.h"
#include <kapplication.h>
#include <kfilemetainfo.h>
#include <kio/global.h>
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kio/netaccess.h>
#include <kmdcodec.h>
#include <kurl.h>
#include <qfile.h> //decodePath()
#include <qcstring.h>
#include <qstring.h>
#include <taglib/fileref.h>

#include <config.h>

#include "metabundle.h"

MetaBundleSaver::MetaBundleSaver( MetaBundle *bundle )
    : QObject()
    , m_bundle( bundle )
{
    DEBUG_BLOCK
}

MetaBundleSaver::~MetaBundleSaver()
{
    DEBUG_BLOCK
    delete m_saveFileref;
}

/*
bool
MetaBundleSaver::scannerSafeSave( TagLib::File* file )
{

    //NOTE: this function will probably be changed around a bit later, as the algorithm may be obsoleted
    //by that in doSave...alternately it maybe should be kept to help reduce contention when writing to a file
    //between amarokapp and the collection scanner
    //note however that this save is not *yet* scanner safe with ATF turned on...
    DEBUG_BLOCK
    if( ( QString( kapp->name() ) == QString( "amarokcollectionscanner" ) ) //default
            || !ScanController::instance() //no scan, we're good
            || !AmarokConfig::advancedTagFeatures() ) //if no ATF, we're not writing to files with scanner
    {
        return file->save();
        //return doSave( file );
    }

    m_safeToSave = false;

    if( ScanController::instance() ) //yes check again, it can pull out from under us at any time
    {
        ScanController::instance()->notifyThisBundle( this );
        if( !ScanController::instance()->requestPause() )
        {
            debug() << "DCOP call to pause scanner failed, aborting save" << endl;
            return false;
        }
    }
    else //scanner seems to have exited in the interim
    {
        return file->save();
        //return doSave( file );
    }

    int count = 0;
    bool result = false;

    debug() << "entering loop to wait on scanner" << endl;

    while( ScanController::instance() && !m_safeToSave && count < 50 ) //time out after five seconds, just in case
    {
        kapp->processEvents( 100 );
        usleep( 100000 );
        count++;
        if( count % 10 == 0 )
            debug() << "waitcount is " << count << endl;
    }

    if( m_safeToSave )
    {
        debug() << "Starting tag save" << endl;
        result = file->save();
        //result = doSave( file );
        debug() << "done, result is " << (result?"success":"failure") << endl;
    }
    else
        debug() << "Did not write tag for file " << m_bundle->url().path() << endl;

    ScanController::instance()->notifyThisBundle( 0 );
    if( !ScanController::instance()->requestUnpause() )
    {
        debug() << "DCOP call to unpause scanner failed, it may be hung" << endl;
        //TODO: Have ScanController kill it and restart?
    }

    return result;
}
*/

TagLib::FileRef *
MetaBundleSaver::prepareToSave()
{
    DEBUG_BLOCK
    const KURL origPath = m_bundle->url();
    char buf[32];
    int hostname = gethostname(buf, 32);
    if( hostname != 0 )
    {
        abortSave( "Could not determine hostname!" );
        return 0;
    }
    QString pid;
    m_tempSavePath = origPath.path() + ".amaroktemp.host-" + QString( buf ) + ".pid-" + pid.setNum( getpid() ) + "." + m_bundle->type();
    m_origRenamedSavePath = origPath.path() + ".amarokoriginal.host-" + QString( buf ) + ".pid-" + pid.setNum( getpid() ) + "." + m_bundle->type();

    KIO::FileCopyJob *copyjob;
    QFile *tempFile;
    QCString tempDigest, origRenamedDigest;

    copyjob = KIO::file_copy( m_bundle->url(), KURL::fromPathOrURL( m_tempSavePath ), -1, false, false, false );
    connect( copyjob, SIGNAL( result(KIO::Job*) ), SLOT( kioDone(KIO::Job*) ) );
    m_waitingOnKIO = 1;

    while( m_waitingOnKIO == 1 )
        kapp->processEvents();

    if( m_waitingOnKIO != 0 )
    {
        abortSave( "Could not create copy!" );
        return 0;
    }

    //By this point, we have the following:
    //The original file is copied at path m_tempSavePath
    //We have generated what will be the filename to rename the original to in m_origRenamedSavePath
    //We have successfully copied the original file to the temp location

    debug() << "Calculating MD5 of " << m_tempSavePath << endl;

    tempFile = new QFile( m_tempSavePath );
    tempFile->open( IO_ReadOnly );
    KMD5* md5sum = new KMD5( tempFile->readAll() );
    tempFile->close();
    delete tempFile;
    m_tempSaveDigest = md5sum->hexDigest();

    debug() << "MD5 sum of temp file: " << m_tempSaveDigest.data() << endl;

    //Now, we have a MD5 sum of the original file at the time of copying saved in m_tempSaveDigest
    //Create a fileref on the copied file, for modification

    m_saveFileref = new TagLib::FileRef( QFile::encodeName( m_tempSavePath ), false );

    if( m_saveFileref && !m_saveFileref->isNull() )
        return m_saveFileref;

    abortSave( "Error creating temp file's fileref!" );
    return 0;
}

bool
MetaBundleSaver::doSave()
{
    //TODO: much commenting needed.  For now this pretty much follows algorithm laid out in bug 131353,
    //but isn't useable since I need to find a good way to switch the file path with taglib, or a good way
    //to get all the metadata copied over.

    DEBUG_BLOCK
    bool revert = false;

    KIO::SimpleJob *deletejob, *deletejob2;
    KIO::FileCopyJob *movejob, *movejob2, *movejob3;
    QFile* origRenamedFile;
    KMD5* md5sum;

    QCString origRenamedDigest;

    //We've made our changes to the fileref; save it first, then do the logic to move the correct file back
    if( !m_saveFileref->save() )
    {
        abortSave( "Could not save the new file!" );
        goto fail_remove_copy;
    }

    movejob = KIO::file_move( m_bundle->url(), KURL::fromPathOrURL( m_origRenamedSavePath ), -1, false, false, false );
    connect( movejob, SIGNAL( result(KIO::Job*) ), SLOT( kioDone(KIO::Job*) ) );
    m_waitingOnKIO = 1;

    while( m_waitingOnKIO == 1 )
        kapp->processEvents();

    if( m_waitingOnKIO != 0 )
    {
        abortSave( "Could not move original!" );
        goto fail_remove_copy;
    }

    origRenamedFile = new QFile( m_origRenamedSavePath );
    origRenamedFile->open( IO_ReadOnly );
    md5sum = new KMD5( origRenamedFile->readAll() );
    origRenamedFile->close();
    delete origRenamedFile;
    origRenamedDigest = md5sum->hexDigest();

    debug() << "md5sum of original file: " << origRenamedDigest.data() << endl;

    if( origRenamedDigest != m_tempSaveDigest )
    {
        abortSave( "Original checksum did not match current checksum!" );
        revert = true;
        goto fail_remove_copy;
    }

    movejob2 = KIO::file_move( KURL::fromPathOrURL( m_tempSavePath ), m_bundle->url(), -1, false, false, false );
    connect( movejob2, SIGNAL( result(KIO::Job*) ), SLOT( kioDone(KIO::Job*) ) );
    m_waitingOnKIO = 1;

    while( m_waitingOnKIO == 1 )
        kapp->processEvents();

    if( m_waitingOnKIO != 0 )
    {
        abortSave( "Could not rename newly-tagged file to original!" );
        revert = true;
        goto fail_remove_copy;
    }

    deletejob = KIO::file_delete( KURL::fromPathOrURL( m_origRenamedSavePath ), false );
    connect( deletejob, SIGNAL( result(KIO::Job*) ), SLOT( kioDone(KIO::Job*) ) );
    m_waitingOnKIO = 1;

    while( m_waitingOnKIO == 1 )
        kapp->processEvents();

    if( m_waitingOnKIO != 0 )
    {
        abortSave( "Could not delete the original file!" );
        amaroK::StatusBar::instance()->longMessageThreadSafe( QString( "Could not remove the copy of the original file, located at %1.  Please remove it manually." ).arg( m_origRenamedSavePath ) );
        return false;
    }

    debug() << "doSave returning true!" << endl;

    return true;

    fail_remove_copy:
        deletejob2 = KIO::file_delete( KURL::fromPathOrURL( m_tempSavePath ), false );
        connect( deletejob2, SIGNAL( result(KIO::Job*) ), SLOT( kioDone(KIO::Job*) ) );
        m_waitingOnKIO = 1;

        while( m_waitingOnKIO == 1 )
            kapp->processEvents();

        if( m_waitingOnKIO != 0 )
        {
            abortSave( "Could not delete the temporary file!" );
            amaroK::StatusBar::instance()->longMessageThreadSafe( QString( "Could not remove the temporary file %1.  Please remove it manually." ).arg( m_tempSavePath ) );
        }

        if( !revert )
            return false;

        movejob3 = KIO::file_move( KURL::fromPathOrURL( m_origRenamedSavePath ), m_bundle->url(), -1, false, false, false );
        connect( movejob3, SIGNAL( result(KIO::Job*) ), SLOT( kioDone(KIO::Job*) ) );
        m_waitingOnKIO = 1;

        while( m_waitingOnKIO == 1 )
            kapp->processEvents();

        if( m_waitingOnKIO != 0 )
        {
            abortSave( "Could not revert file to original filename!" );
            amaroK::StatusBar::instance()->longMessageThreadSafe( QString( "Could not revert the file to its original filename, from %1 to %2.  Please do this manually." ).arg( m_origRenamedSavePath ).arg( m_bundle->url().path() ) );
        }

        return false;
}

bool
MetaBundleSaver::cleanupSave()
{
    DEBUG_BLOCK

    bool dirty = false;

    if( !(m_tempSavePath == QString::null) && QFile::exists( m_tempSavePath ) && !QFile::remove( m_tempSavePath ) )
    {
        dirty = true;
        abortSave( "Could not delete the temporary file!" );
        amaroK::StatusBar::instance()->longMessageThreadSafe( QString( "Could not remove the temporary file %1.  Please remove it manually." ).arg( m_tempSavePath ) );
    }

    m_tempSavePath = QString::null;
    m_origRenamedSavePath = QString::null;
    m_tempSaveDigest = QCString( 0 );
    delete m_saveFileref;

    return !dirty;
}

void
MetaBundleSaver::kioDone( KIO::Job *job )
{
    DEBUG_BLOCK
    if( job->error() )
    {
        debug() << "Error! Code is: " << job->error() << endl;
        m_waitingOnKIO = -1;
        return;
    }
    m_waitingOnKIO = 0;
}

void
MetaBundleSaver::abortSave( const QString message  )
{
    DEBUG_BLOCK
    //TODO: maybe make this pop up in the status bar?  What if there are a lot of problems?
    debug() << message << endl;
}

#include "metabundlesaver.moc"

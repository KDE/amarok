/***************************************************************************
    copyright            : (C) 2004 by Scott Wheeler
    email                : wheeler@kde.org
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it  under the terms of the GNU Lesser General Public License version  *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307  *
 *   USA                                                                   *
 ***************************************************************************/
#include "config.h"
#include "debug.h"
#include "statusbar.h"
#define DEBUG_PREFIX "KTRM"

#include "ktrm.h"

#include <kapplication.h>
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kprotocolmanager.h>
#include <kurl.h>
#include <kresolver.h>

#include <qdom.h>
#include <qmutex.h>
#include <qevent.h>
#include <qobject.h>
#include <qfile.h>
#include <qregexp.h>

#if HAVE_TUNEPIMP

#if HAVE_TUNEPIMP >= 5
#include <tunepimp-0.5/tp_c.h>
#else
#include <tunepimp/tp_c.h>
#endif

class KTRMLookup;

extern "C"
{
#if HAVE_TUNEPIMP >= 4
    static void TRMNotifyCallback(tunepimp_t pimp, void *data, TPCallbackEnum type, int fileId, TPFileStatus status);
#else
    static void TRMNotifyCallback(tunepimp_t pimp, void *data, TPCallbackEnum type, int fileId);
#endif
}

/**
 * This represents the main TunePimp instance and handles incoming requests.
 */

class KTRMRequestHandler
{
public:
    static KTRMRequestHandler *instance()
    {
        static QMutex mutex;
        mutex.lock();
        static KTRMRequestHandler handler;
        mutex.unlock();
        return &handler;
    }

    int startLookup(KTRMLookup *lookup)
    {
        int id;
        if(!m_fileMap.contains(lookup->file())) {
#if HAVE_TUNEPIMP >= 4
            id = tp_AddFile(m_pimp, QFile::encodeName(lookup->file()), 0);
#else
            id = tp_AddFile(m_pimp, QFile::encodeName(lookup->file()));
#endif
            m_fileMap.insert(lookup->file(), id);
        }
        else {
            id = m_fileMap[lookup->file()];
            tp_IdentifyAgain(m_pimp, id);
        }
        m_lookupMap[id] = lookup;
        return id;
    }

    void endLookup(KTRMLookup *lookup)
    {
        tp_ReleaseTrack(m_pimp, tp_GetTrack(m_pimp, lookup->fileId()));
        tp_Remove(m_pimp, lookup->fileId());
        m_lookupMapMutex.lock();
        m_lookupMap.remove(lookup->fileId());
        m_fileMap.remove( lookup->file() );
        m_lookupMapMutex.unlock();
    }

    bool lookupMapContains(int fileId) const
    {
        m_lookupMapMutex.lock();
        bool contains = m_lookupMap.contains(fileId);
        m_lookupMapMutex.unlock();
        return contains;
    }

    KTRMLookup *lookup(int fileId) const
    {
        m_lookupMapMutex.lock();
        KTRMLookup *l = m_lookupMap[fileId];
        m_lookupMapMutex.unlock();
        return l;
    }

    void removeFromLookupMap(int fileId)
    {
        m_lookupMapMutex.lock();
        m_lookupMap.remove(fileId);
        m_lookupMapMutex.unlock();
    }

    const tunepimp_t &tunePimp() const
    {
        return m_pimp;
    }

protected:
    KTRMRequestHandler()
    {
        m_pimp = tp_New("KTRM", "0.1");
        //tp_SetDebug(m_pimp, true);
#if HAVE_TUNEPIMP < 5
        tp_SetTRMCollisionThreshold(m_pimp, 100);
        tp_SetAutoFileLookup(m_pimp,true);
#endif
        tp_SetAutoSaveThreshold(m_pimp, -1);
        tp_SetMoveFiles(m_pimp, false);
        tp_SetRenameFiles(m_pimp, false);
#if HAVE_TUNEPIMP >= 4
        tp_SetFileNameEncoding(m_pimp, "UTF-8");
#else
        tp_SetUseUTF8(m_pimp, true);
#endif
        tp_SetNotifyCallback(m_pimp, TRMNotifyCallback, 0);

#if HAVE_TUNEPIMP < 5
        KProtocolManager::reparseConfiguration();

        if(KProtocolManager::useProxy()) {
            QString noProxiesFor = KProtocolManager::noProxyFor();
            QStringList noProxies = QStringList::split(QRegExp("[',''\t'' ']"), noProxiesFor);
            bool useProxy = true;

            char server[255];
            short port;
            tp_GetServer(m_pimp, server, 255, &port);
            QString tunepimpHost = QString(server);
            QString tunepimpHostWithPort = (tunepimpHost + ":%1").arg(port);

            for(QStringList::ConstIterator it = noProxies.constBegin(); it != noProxies.constEnd(); ++it) {
                QString normalizedHost = KNetwork::KResolver::normalizeDomain(*it);
                if(normalizedHost == tunepimpHost ||
                    tunepimpHost.endsWith('.' + normalizedHost)) {
                    useProxy = false;
                    break;
                }

                if(normalizedHost == tunepimpHostWithPort ||
                    tunepimpHostWithPort.endsWith('.' + normalizedHost)) {
                    useProxy = false;
                    break;
                }
            }

            if(KProtocolManager::useReverseProxy())
                useProxy = !useProxy;

            if(useProxy) {
                KURL proxy = KProtocolManager::proxyFor("http");
                tp_SetProxy(m_pimp, proxy.host().latin1(), short(proxy.port()));
            }
        }
#else
        tp_SetMusicDNSClientId(m_pimp, "0c6019606b1d8a54d0985e448f3603ca");
#endif
    }

    ~KTRMRequestHandler()
    {
        tp_Delete(m_pimp);
    }

private:
    tunepimp_t m_pimp;
    QMap<int, KTRMLookup *> m_lookupMap;
    QMap<QString, int> m_fileMap;
    mutable QMutex m_lookupMapMutex;
};


/**
 * A custom event type used for signalling that a TRM lookup is finished.
 */

class KTRMEvent : public QCustomEvent
{
public:
    enum Status {
        Recognized,
        Unrecognized,
        Collision,
        PuidGenerated,
        Error
    };

    KTRMEvent(int fileId, Status status) :
        QCustomEvent(id),
        m_fileId(fileId),
        m_status(status) {}

    int fileId() const
    {
        return m_fileId;
    }

    Status status() const
    {
        return m_status;
    }

    static const int id = User + 1984; // random, unique, event id

private:
    int m_fileId;
    Status m_status;
};

/**
 * A helper class to intercept KTRMQueryEvents and call recognized() (from the GUI
 * thread) for the lookup.
 */

class KTRMEventHandler : public QObject
{
public:
    static void send(int fileId, KTRMEvent::Status status)
    {
        KApplication::postEvent(instance(), new KTRMEvent(fileId, status));
    }

protected:
    KTRMEventHandler() : QObject() {}

    static KTRMEventHandler *instance()
    {
        static QMutex mutex;
        mutex.lock();
        static KTRMEventHandler handler;
        mutex.unlock();
        return &handler;
    }

    virtual void customEvent(QCustomEvent *event)
    {
        if(!event->type() == KTRMEvent::id)
            return;

        KTRMEvent *e = static_cast<KTRMEvent *>(event);

        static QMutex mutex;
        mutex.lock();

        if(!KTRMRequestHandler::instance()->lookupMapContains(e->fileId())) {
            mutex.unlock();
            return;
        }

        KTRMLookup *lookup = KTRMRequestHandler::instance()->lookup(e->fileId());
#if HAVE_TUNEPIMP >= 4
        if ( e->status() != KTRMEvent::Unrecognized)
#endif
            KTRMRequestHandler::instance()->removeFromLookupMap(e->fileId());

        mutex.unlock();

        switch(e->status()) {
        case KTRMEvent::Recognized:
            lookup->recognized();
            break;
        case KTRMEvent::Unrecognized:
            lookup->unrecognized();
            break;
        case KTRMEvent::Collision:
            lookup->collision();
            break;
        case KTRMEvent::PuidGenerated:
            lookup->puidGenerated();
            break;
        case KTRMEvent::Error:
            lookup->error();
            break;
        }
    }
};

/**
 * Callback function for TunePimp lookup events.
 */
#if HAVE_TUNEPIMP >= 4
static void TRMNotifyCallback(tunepimp_t /*pimp*/, void */*data*/, TPCallbackEnum type, int fileId, TPFileStatus status)
#else
static void TRMNotifyCallback(tunepimp_t pimp, void */*data*/, TPCallbackEnum type, int fileId)
#endif
{
    if(type != tpFileChanged)
        return;

#if HAVE_TUNEPIMP < 4
    track_t track = tp_GetTrack(pimp, fileId);
    TPFileStatus status = tr_GetStatus(track);
#endif
    //debug() << "Status is: " << status << endl;

    switch(status) {
    case eRecognized:
        KTRMEventHandler::send(fileId, KTRMEvent::Recognized);
        break;
    case eUnrecognized:
        KTRMEventHandler::send(fileId, KTRMEvent::Unrecognized);
        break;
#if HAVE_TUNEPIMP >= 5
    case ePUIDLookup:
    case ePUIDCollision:
    case eFileLookup:
        KTRMEventHandler::send(fileId, KTRMEvent::PuidGenerated);
        break;
#else
    case eTRMCollision:
#if HAVE_TUNEPIMP >= 4
    case eUserSelection:
#endif
        KTRMEventHandler::send(fileId, KTRMEvent::Collision);
        break;
#endif
    case eError:
        KTRMEventHandler::send(fileId, KTRMEvent::Error);
        break;
    default:
        break;
    }
#if HAVE_TUNEPIMP < 4
    tp_ReleaseTrack(pimp, track);
#endif
}

////////////////////////////////////////////////////////////////////////////////
// KTRMResult implementation
////////////////////////////////////////////////////////////////////////////////

class KTRMResult::KTRMResultPrivate
{
public:
    KTRMResultPrivate() : track(0), year(0), relevance(0) {}
    QString title;
    QString artist;
    QString album;
    int track;
    int year;
    double relevance;

    bool operator== (const KTRMResultPrivate &r) const;
};

bool KTRMResult::KTRMResultPrivate::operator==(const KTRMResultPrivate &r) const
{
	return (
			title == r.title &&
			artist == r.artist &&
			album == r.album &&
			track == r.track &&
			year == r.year &&
			relevance == r.relevance
	       );
}

////////////////////////////////////////////////////////////////////////////////
// KTRMResult public methods
////////////////////////////////////////////////////////////////////////////////
#endif
KTRMResult::KTRMResult()
{
#if HAVE_TUNEPIMP
    d = new KTRMResultPrivate;
#endif
}

KTRMResult::KTRMResult(const KTRMResult &result)
{
#if HAVE_TUNEPIMP
    d = new KTRMResultPrivate(*result.d);
#else
    Q_UNUSED(result);
#endif
}

KTRMResult::~KTRMResult()
{
#if HAVE_TUNEPIMP
    delete d;
#endif
}

QString KTRMResult::title() const
{
#if HAVE_TUNEPIMP
    return d->title;
#else
    return QString();
#endif
}

QString KTRMResult::artist() const
{
#if HAVE_TUNEPIMP
    return d->artist;
#else
    return QString();
#endif
}

QString KTRMResult::album() const
{
#if HAVE_TUNEPIMP
    return d->album;
#else
    return QString();
#endif
}

int KTRMResult::track() const
{
#if HAVE_TUNEPIMP
    return d->track;
#else
    return 0;
#endif
}

int KTRMResult::year() const
{
#if HAVE_TUNEPIMP
    return d->year;
#else
    return 0;
#endif
}

bool KTRMResult::operator<(const KTRMResult &r) const
{
#if HAVE_TUNEPIMP
    return r.d->relevance < d->relevance;
#else
    Q_UNUSED(r);
    return false;
#endif
}

bool KTRMResult::operator>(const KTRMResult &r) const
{
#if HAVE_TUNEPIMP
    return r.d->relevance > d->relevance;
#else
    Q_UNUSED(r);
    return true;
#endif
}

KTRMResult &KTRMResult::operator= (const KTRMResult &r)
{
#if HAVE_TUNEPIMP
    d = new KTRMResultPrivate(*r.d);
#else
    Q_UNUSED(r);
#endif
    return *this;
}

bool KTRMResult::operator== (const KTRMResult &r) const
{
#if HAVE_TUNEPIMP
    return *d == *(r.d);
#else
    Q_UNUSED(r);
#endif
    return false;
}


bool KTRMResult::isEmpty() const
{
#if HAVE_TUNEPIMP
    return d->title.isEmpty() && d->artist.isEmpty() && d->album.isEmpty() &&
        d->track == 0 && d->year == 0;
#else
    return true;
#endif
}
#if HAVE_TUNEPIMP
////////////////////////////////////////////////////////////////////////////////
// KTRMLookup implementation
////////////////////////////////////////////////////////////////////////////////

class KTRMLookup::KTRMLookupPrivate
{
public:
    KTRMLookupPrivate() :
        fileId(-1), autoDelete(false) {}
    QString file;
    QString errorString;
    KTRMResultList results;
    int fileId;
    bool autoDelete;
};
#endif
////////////////////////////////////////////////////////////////////////////////
// KTRMLookup public methods
////////////////////////////////////////////////////////////////////////////////

KTRMLookup::KTRMLookup(const QString &file, bool autoDelete)
    : QObject()
{
#if HAVE_TUNEPIMP
    d = new KTRMLookupPrivate;
    d->file = file;
    d->autoDelete = autoDelete;
    d->fileId = KTRMRequestHandler::instance()->startLookup(this);
#else
    Q_UNUSED(file);
    Q_UNUSED(autoDelete);
#endif
}

KTRMLookup::~KTRMLookup()
{
#if HAVE_TUNEPIMP
    KTRMRequestHandler::instance()->endLookup(this);
    delete d;
#endif
}

QString KTRMLookup::file() const
{
#if HAVE_TUNEPIMP
    return d->file;
#else
    return QString();
#endif
}

int KTRMLookup::fileId() const
{
#if HAVE_TUNEPIMP
    return d->fileId;
#else
    return -1;
#endif
}

void KTRMLookup::recognized()
{
#if HAVE_TUNEPIMP
    debug() << k_funcinfo << d->file << endl;

    d->results.clear();

    metadata_t *metaData = md_New();
    track_t track = tp_GetTrack(KTRMRequestHandler::instance()->tunePimp(), d->fileId);
    tr_Lock(track);
    tr_GetServerMetadata(track, metaData);

    KTRMResult result;

    result.d->title = QString::fromUtf8(metaData->track);
    result.d->artist = QString::fromUtf8(metaData->artist);
    result.d->album = QString::fromUtf8(metaData->album);
    result.d->track = metaData->trackNum;
    result.d->year = metaData->releaseYear;

    d->results.append(result);

    md_Delete(metaData);
    tr_Unlock(track);
    finished();
#endif
}

void KTRMLookup::unrecognized()
{
#if HAVE_TUNEPIMP
    debug() << k_funcinfo << d->file << endl;
    #if HAVE_TUNEPIMP >= 4
    char trm[255];
    bool finish = false;
    trm[0] = 0;
    track_t track = tp_GetTrack(KTRMRequestHandler::instance()->tunePimp(), d->fileId);
    tr_Lock(track);
#if HAVE_TUNEPIMP >= 5
    tr_GetPUID(track, trm, 255);
#else
    tr_GetTRM(track, trm, 255);
#endif
    if ( !trm[0] ) {
        tr_SetStatus(track, ePending);
        tp_Wake(KTRMRequestHandler::instance()->tunePimp(), track);
    }
    else
        finish = true;
    tr_Unlock(track);
    tp_ReleaseTrack(KTRMRequestHandler::instance()->tunePimp(), track);
    if ( !finish )
        return;
    #endif
    d->results.clear();
    finished();

#endif
}

void KTRMLookup::collision()
{
#if HAVE_TUNEPIMP && HAVE_TUNEPIMP < 5
    debug() << k_funcinfo << d->file << endl;

    track_t track = tp_GetTrack(KTRMRequestHandler::instance()->tunePimp(), d->fileId);

    if(track <= 0) {
        debug() << "invalid track number" << endl;
        return;
    }

    tr_Lock(track);
    int resultCount = tr_GetNumResults(track);

    QStringList strList = QStringList::split ( '/', d->file );

    metadata_t *mdata = md_New();
    strList.append( QString::fromUtf8(mdata->track) );
    strList.append( QString::fromUtf8(mdata->artist) );
    strList.append( QString::fromUtf8(mdata->album) );
    md_Clear(mdata);

    if(resultCount > 0) {
        TPResultType type;
        result_t *results = new result_t[resultCount];
        tr_GetResults(track, &type, results, &resultCount);

        switch(type) {
        case eNone:
            debug() << k_funcinfo << "eNone" << endl;
            break;
        case eArtistList:
            debug() << "eArtistList" << endl;
            break;
        case eAlbumList:
            debug() << "eAlbumList" << endl;
            break;
        case eTrackList:
        {
            debug() << "eTrackList" << endl;
            albumtrackresult_t **tracks = reinterpret_cast<albumtrackresult_t **>( results );
            d->results.clear();

            for(int i = 0; i < resultCount; i++) {
                KTRMResult result;

                result.d->title = QString::fromUtf8(tracks[i]->name);
#if HAVE_TUNEPIMP >= 4
                result.d->artist = QString::fromUtf8(tracks[i]->artist.name);
                result.d->album = QString::fromUtf8(tracks[i]->album.name);
                result.d->year = tracks[i]->album.releaseYear;
#else
                result.d->artist = QString::fromUtf8(tracks[i]->artist->name);
                result.d->album = QString::fromUtf8(tracks[i]->album->name);
                result.d->year = tracks[i]->album->releaseYear;
#endif
                result.d->track = tracks[i]->trackNum;
                result.d->relevance =
                    4 * stringSimilarity(strList,result.d->title) +
                    2 * stringSimilarity(strList,result.d->artist) +
                    1 * stringSimilarity(strList,result.d->album);

                if(!d->results.contains(result)) d->results.append(result);
            }
            break;
        }
        case eMatchedTrack:
            debug() << k_funcinfo << "eMatchedTrack" << endl;
            break;
        }

        delete [] results;
    }

    tr_Unlock(track);
    tp_ReleaseTrack(KTRMRequestHandler::instance()->tunePimp(), track);
    qHeapSort(d->results);

    finished();
#endif
}

void KTRMLookup::puidGenerated()
{
#if HAVE_TUNEPIMP >= 5
    DEBUG_BLOCK
    debug() << k_funcinfo << d->file << endl;
    char puid[255] = {0};
    track_t track = tp_GetTrack(KTRMRequestHandler::instance()->tunePimp(), d->fileId);
    tr_Lock(track);

    tr_GetPUID(track, puid, 255);
    debug() << puid << endl;
    tr_Unlock(track);
    tp_ReleaseTrack(KTRMRequestHandler::instance()->tunePimp(), track);
    d->results.clear();

    KIO::Job *job = KIO::storedGet( QString( "http://musicbrainz.org/ws/1/track/?type=xml&puid=%1" ).arg( puid ) , false, false );

    Amarok::StatusBar::instance()->newProgressOperation( job )
            .setDescription( i18n( "MusicBrainz Lookup" ) );

    connect( job, SIGNAL( result( KIO::Job* ) ), SLOT( lookupResult( KIO::Job* ) ) );
#endif
}

void KTRMLookup::lookupResult( KIO::Job* job )
{
#if HAVE_TUNEPIMP >= 5
    DEBUG_BLOCK
    if ( !job->error() == 0 ) {
        warning() << "[MusicBrainzLookup] KIO error! errno: " << job->error() << endl;
        Amarok::StatusBar::instance()->longMessage( "Couldn't connect to MusicBrainz server." );
        finished();
        return;
    }
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QString xml = QString::fromUtf8( storedJob->data().data(), storedJob->data().size() );

    QDomDocument doc;
    QDomElement e;

    if( !doc.setContent( xml ) ) {
        warning() << "[MusicBrainzLookup] Invalid XML" << endl;
        Amarok::StatusBar::instance()->longMessage( "MusicBrainz returned invalid content." );
        finished();
        return;
    }

    e = doc.namedItem( "metadata" ).toElement().namedItem( "track-list" ).toElement();

    QStringList strList = QStringList::split ( '/', d->file );

    QDomNode n = e.namedItem("track");
    for( ; !n.isNull();  n = n.nextSibling() ) {
        QDomElement track = n.toElement();
        KTRMResult result;

        result.d->title = track.namedItem( "title" ).toElement().text();
        result.d->artist = track.namedItem( "artist" ).toElement().namedItem( "name" ).toElement().text();
        QDomNode releaseNode = track.namedItem("release-list").toElement().namedItem("release");
        for( ; !releaseNode.isNull();  releaseNode = releaseNode.nextSibling() ) {
            KTRMResult tmpResult( result );
            QDomElement release = releaseNode.toElement();

            tmpResult.d->album = release.namedItem( "title" ).toElement().text();
            QDomNode tracklistN = release.namedItem( "track-list" );
            if ( !tracklistN.isNull() ) {
                QDomElement tracklist = tracklistN.toElement();
                if ( !tracklist.attribute( "offset" ).isEmpty() )
                    tmpResult.d->track = tracklist.attribute( "offset" ).toInt() + 1;
            }
            //tmpResult.d->year = ???;
            tmpResult.d->relevance =
                4 * stringSimilarity(strList,tmpResult.d->title) +
                2 * stringSimilarity(strList,tmpResult.d->artist) +
                1 * stringSimilarity(strList,tmpResult.d->album);
            if( !d->results.contains( tmpResult ) )
                d->results.append( tmpResult );
        }
     }

     qHeapSort(d->results);

     finished();
#else
    Q_UNUSED( job );
#endif
}

void KTRMLookup::error()
{
#if HAVE_TUNEPIMP
    debug() << k_funcinfo << d->file << endl;
    track_t track = tp_GetTrack(KTRMRequestHandler::instance()->tunePimp(), d->fileId);
    char error[1000];
    tr_GetError( track, error, 1000);
    debug() << "Error: " << error << endl;
    d->errorString = error;
    d->results.clear();
    finished();
#endif
}

KTRMResultList KTRMLookup::results() const
{
#if HAVE_TUNEPIMP
    return d->results;
#else
    return KTRMResultList();
#endif
}

////////////////////////////////////////////////////////////////////////////////
// KTRMLookup protected methods
////////////////////////////////////////////////////////////////////////////////

void KTRMLookup::finished()

{
#if HAVE_TUNEPIMP
    emit sigResult( results(), d->errorString );

    if(d->autoDelete)
        deleteLater();
#endif
}

////////////////////////////////////////////////////////////////////////////////
// Helper Functions used for sorting MusicBrainz results
////////////////////////////////////////////////////////////////////////////////
double stringSimilarity(QStringList &l, QString &s)
{
    double max = 0, current = 0;
    for ( QStringList::Iterator it = l.begin(); it != l.end(); ++it ) {
       if( max < (current = stringSimilarity((*it),s)))
            max = current;
    }
    return max;
}

double stringSimilarity(QString s1, QString s2)
{
    s1.remove( QRegExp("[\\s\\t\\r\\n]") );
    s2.remove( QRegExp("[\\s\\t\\r\\n]") );

    double nCommon = 0;
    int p1 = 0, p2 = 0, x1 = 0, x2 = 0;
    int l1 = s1.length(), l2 = s2.length(), l3 = l1 + l2;
    QChar c1 = 0, c2 = 0;

    while(p1 < l1 && p2 < l2) {
        c1 = s1.at(p1); c2 = s2.at(p2);
        if( c1.upper() == c2.upper()) {
            ++nCommon;
            ++p1; ++p2;
        }
        else {
            x1 = s1.find(c2,p1,false);
            x2 = s2.find(c1,p2,false);

            if( (x1 == x2 || -1 == x1) || (-1 != x2 && x1 > x2) )
                ++p2;
            else
                ++p1;
        }
    }
    return l3 ? (double)(nCommon*2) / (double)(l3) : 1;
}





#include "ktrm.moc"


// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Stefan Bogner <bochi@online.ms>
// See COPYING file for licensing information.

#ifndef AMAROK_COVERFETCHER_H
#define AMAROK_COVERFETCHER_H

#include <qimage.h>       //stack allocated
#include <qobject.h>      //baseclass
#include <qstringlist.h>  //stack allocated

namespace KIO { class Job; }

class CoverFetcher : public QObject
{
   Q_OBJECT

   static const uint BUFFER_SIZE = 2000000; // 2mb
   static const uint MAX_COVERS_CHOICE = 10;

public:
    CoverFetcher( QWidget *parent, QString artist, QString album );
   ~CoverFetcher();

    /// allow the user to edit the query?
    void setUserCanEditQuery( bool b ) { m_userCanEditQuery = b; }

    /// starts the fetch
    void startFetch();

    QString artist() const { return m_artist; }
    QString album() const { return m_album; }
    QString amazonURL() const { return m_amazonURL; }
    QImage image() const { return m_image; }

    bool wasError() const { return !m_errors.isEmpty(); }
    QStringList errors() const { return m_errors; }

signals:
    /// The CollectionDB can get the cover information using the pointer
    void result( CoverFetcher* );

private slots:
    void receivedXmlData( KIO::Job* job, const QByteArray& data );
    void finishedXmlFetch( KIO::Job* job );
    void receivedImageData( KIO::Job* job, const QByteArray& data );
    void finishedImageFetch( KIO::Job* job );

private:
    const QString m_artist;
    const QString m_album;

    bool    m_userCanEditQuery;
    QString m_userQuery; /// the query from the query edit dialog
    QString m_xml;
    QImage  m_image;
    QString m_amazonURL;
    uchar  *m_buffer;
    uint    m_bufferIndex;
    int     m_size;

    QStringList m_queries;
    QStringList m_coverUrls;
    QStringList m_coverNames;
    QString     m_currentCoverName;
    QStringList m_errors;

private:
    /// The fetch was successful!
    void finish();

    /// The fetch failed, finish up and log an error message
    void finishWithError( const QString &message, KIO::Job *job = 0 );

    /// Will try all available queries, and then prompt the user, if allowed
    void attemptAnotherFetch();

    /// Prompt the user for a query
    void getUserQuery( QString explanation = QString::null );

    /// Show the cover that has been found
    void showCover();
};

#endif /* AMAROK_COVERFETCHER_H */

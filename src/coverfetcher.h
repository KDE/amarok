// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Stefan Bogner <bochi@online.ms>
// See COPYING file for licensing information.

#ifndef AMAROK_COVERFETCHER_H
#define AMAROK_COVERFETCHER_H

#define MAX_COVERS_CHOICE  10

#include <qimage.h>       //stack allocated
#include <qobject.h>      //baseclass
#include <qstringlist.h>  //stack allocated

namespace KIO { class Job; }

class CoverFetcher : public QObject
{
   Q_OBJECT

   static const uint BUFFER_SIZE = 2000000; // 2mb

public:
   CoverFetcher( QWidget *parent, QString artist, QString album );
  ~CoverFetcher();

   /// allow the user to edit the query?
   void setUserCanEditQuery( bool b ) { m_userCanEditQuery = b; }

   /// starts the fetch
   void startFetch();

   /// @param the text to show in the dialog, there is a default text
   void showQueryEditor( QString text = QString::null );

public:
    QString artist() const { return m_artist; }
    QString album() const { return m_album; }
    QString amazonURL() const { return m_amazonURL; }
    QImage image() const { return m_image; }

    bool error() const { return !m_errorMessage.isEmpty(); }
    QString errorMessage() const { return m_errorMessage; }

signals:
    /// The CollectionDB can get the cover information using the pointer
    void result( CoverFetcher* );

private slots:
    void receivedXmlData( KIO::Job* job, const QByteArray& data );
    void finishedXmlFetch( KIO::Job* job );
    void receivedImageData( KIO::Job* job, const QByteArray& data );
    void finishedImageFetch( KIO::Job* job );

    void finish();

private:
    const QString m_artist;
    const QString m_album;
    QStringList   m_queries;

    bool    m_userCanEditQuery;
    QString m_userQuery;
    QString m_fetchedXML;
    QImage  m_image;

    QString m_amazonURL;
    QString m_imageURL[MAX_COVERS_CHOICE];
    uint    m_iCoverNbr;
    uint    m_iCover;

    uchar  *m_buffer;
    uint    m_bufferIndex;

    int     m_size;

    QString m_errorMessage;

private:
    /// the fetch failed, exit with error message
    void error( const QString &message, KIO::Job *job = 0 );
    
    // run the cover query job
    void query_cover( int iCoverIndex );
};

#endif /* AMAROK_COVERFETCHER_H */

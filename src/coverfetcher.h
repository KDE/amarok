// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Stefan Bogner <bochi@online.ms>
// See COPYING file for licensing information.

#ifndef AMAROK_COVERFETCHER_H
#define AMAROK_COVERFETCHER_H

#include <qlabel.h>       //baseclass
#include <qimage.h>       //stack allocated
#include <qobject.h>      //baseclass
#include <qstringlist.h>  //stack allocated
#include <qdom.h>     //stack allocated

namespace Amarok {
    void coverContextMenu(  QWidget *parent, QPoint point, const QString &artist, const QString &album, bool showCoverManager = true );
}


class CoverLabel : public QLabel {
    public:
    CoverLabel ( QWidget * parent, const char * name = 0, WFlags f = 0 );

    void setInformation( const QString artist, const QString album ) {
        m_artist = artist;
        m_album = album;
    }

    protected:
    virtual void mouseReleaseEvent(QMouseEvent *pEvent);

    private:
    QString m_artist;
    QString m_album;
};



namespace KIO { class Job; }

class CoverFetcher : public QObject
{
    friend class EditSearchDialog;
    Q_OBJECT

    static const uint MAX_COVERS_CHOICE = 10;

public:
    CoverFetcher( QWidget *parent, const QString &artist, QString album );
   ~CoverFetcher();

    /// allow the user to edit the query?
    void setUserCanEditQuery( bool b ) { m_userCanEditQuery = b; }

    /// starts the fetch
    void startFetch();

    QString artist() const { return m_artist; }
    QString album() const { return m_album; }
    QString amazonURL() const { return m_amazonURL; }
    QString asin() const { return m_asin; }
    QImage image() const { return m_image; }

    bool wasError() const { return !m_success; }
    QStringList errors() const { return m_errors; }

    enum Locale { International=0, France, Germany, Japan, UK, Canada };
    static QString localeIDToString( int id );
    static int localeStringToID( const QString &locale );

signals:
    /// The CollectionDB can get the cover information using the pointer
    void result( CoverFetcher* );

private slots:
    void finishedXmlFetch( KIO::Job* job );
    void finishedImageFetch( KIO::Job* job );
    void changeLocale( int id );

private:
    const QString m_artist;
    const QString m_album;

    bool    m_userCanEditQuery;
    QString m_userQuery; /// the query from the query edit dialog
    QString m_xml;
    QImage  m_image;
    QString m_amazonURL;
    QString m_asin;
    int     m_size;

    QStringList m_queries;
    QStringList m_coverAsins;
    QStringList m_coverAmazonUrls;
    QStringList m_coverUrls;
    QStringList m_coverNames;
    QString     m_currentCoverName;
    QStringList m_errors;

    bool m_success;

private:
    /// The fetch was successful!
    void finish();

    /// Parse one <Item> QDomNode and append results.
    void parseItemNode( const QDomNode &node );

    /// The fetch failed, finish up and log an error message
    void finishWithError( const QString &message, KIO::Job *job = 0 );

    /// Prompt the user for a query
    void getUserQuery( QString explanation = QString::null );

    /// Will try all available queries, and then prompt the user, if allowed
    void attemptAnotherFetch();

    /// Show the cover that has been found
    void showCover();
};

#endif /* AMAROK_COVERFETCHER_H */

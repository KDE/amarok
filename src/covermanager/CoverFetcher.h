/****************************************************************************************
 * Copyright (c) 2004 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2004 Stefan Bogner <bochi@online.ms>                                   *
 * Copyright (c) 2007 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
 * Copyright (c) 2009 Martin Sandsmark <sandsmark@samfundet.no>                         *
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

#ifndef AMAROK_COVERFETCHER_H
#define AMAROK_COVERFETCHER_H

#include "meta/Meta.h"

#include <QDomNode>
#include <QImage>       //stack allocated
#include <QLabel>       //baseclass
#include <QMutex>
#include <QObject>      //baseclass
#include <QStringList>  //stack allocated

class KJob;
class KLineEdit;

class CoverLabel : public QLabel
{
    public:
    explicit CoverLabel( QWidget * parent, Qt::WindowFlags f = 0 );

    void setInformation( const QString artist, const QString album )
    {
        m_artist = artist;
        m_album = album;
    }

    protected:
        virtual void mouseReleaseEvent(QMouseEvent *pEvent);

    private:
        QString m_artist;
        QString m_album;
};

class CoverFetcherSingleton;

namespace KIO { class Job; }

class CoverFetcher : public QObject
{
    friend class EditSearchDialog;
    Q_OBJECT

    static const uint MAX_COVERS_CHOICE = 10;

public:
    AMAROK_EXPORT static CoverFetcher* instance();
    AMAROK_EXPORT static void destroy();

    /// allow the user to edit the query?
    void setInteractive( bool b ) { m_interactive = b; }

    /// Main Fetch loop
    AMAROK_EXPORT void manualFetch( Meta::AlbumPtr album );

    QPixmap image() const { return m_pixmap; }

    AMAROK_EXPORT void queueAlbum( Meta::AlbumPtr album );
    AMAROK_EXPORT void queueAlbums( Meta::AlbumList albums );

    bool wasError() const { return !m_success; }
    QStringList errors() const { return m_errors; }

private slots:
    void finishedXmlFetch( KJob * job );
    void finishedImageFetch( KJob * job );

private:
    static CoverFetcher* s_instance;
    CoverFetcher();
    ~CoverFetcher();

    void parseItemNode( const QDomNode &node );

    Meta::AlbumList m_albums;
    Meta::AlbumPtr m_albumPtr;
    QMutex m_albumsMutex;
    QMutex m_fetchMutex;

    bool    m_interactive; /// whether we should consult the user
    QString m_userQuery; /// the query from the query edit dialog
    QString m_xml;
    QPixmap  m_pixmap;
    QString m_asin;
    int     m_size;

    QStringList m_queries;
    QString     m_currentCoverName;
    QStringList m_errors;

    bool m_success;
    bool m_isFetching;

private:
    /// Fetch a cover
    void startFetch( Meta::AlbumPtr album );

    /// The fetch was successful!
    void finish();

    /// The fetch failed, finish up and log an error message
    void finishWithError( const QString &message, KJob *job = 0 );

    /// Show the cover that has been found
    void showCover();
};

namespace The
{
    inline CoverFetcher *coverFetcher() { return CoverFetcher::instance(); }
}

#endif /* AMAROK_COVERFETCHER_H */

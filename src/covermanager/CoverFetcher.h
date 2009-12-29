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
#include <KDialog>
#include <KHBox>
#include <KPushButton>
#include <KVBox>

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

    QPixmap image() const { return m_selPixmap; }

    AMAROK_EXPORT void queueAlbum( Meta::AlbumPtr album );
    AMAROK_EXPORT void queueAlbums( Meta::AlbumList albums );

    bool wasError() const { return !m_success; }
    QStringList errors() const { return m_errors; }

    enum FinishState { Success, Error, NotFound };

    /**
     * Available album cover sizes in Last.fm's api.
     */
    enum CoverSize
    {
        Small = 0,  //! 34px
        Medium,     //! 64px
        Large,      //! 128px
        ExtraLarge  //! 300px
    };

signals:
    void finishedSingle( int state );

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
    QList<QPixmap> m_pixmaps;     //!List of found covers
    QPixmap m_selPixmap;          //!Cover of choice
    int     m_processedCovers;    //!number of covers that have been processed
    int     m_numURLS;            //!number of URLS to process
    QString m_asin;

    QStringList m_queries;
    QString     m_currentCoverName;
    QStringList m_errors;

    QHash< QUrl, Meta::AlbumPtr > m_urlMap;

    bool m_success;
    bool m_isFetching;

    /// Fetch a cover
    void startFetch( Meta::AlbumPtr album );

    /// cleanup depending on the fetch result
    void finish( FinishState state = Success, const QString &message = QString(), KJob *job = 0 );

    /// Show the cover that has been found
    void showCover();

    /// convert CoverSize enum to string
    QString coverSizeString( enum CoverSize size ) const;
    
    /// lower, remove whitespace, and do Unicode normalization on a QString
    QString normalizeString( const QString &raw );

    /// lower, remove whitespace, and do Unicode normalization on a QStringList
    QStringList normalizeStrings( const QStringList &rawList );
};


class CoverFoundDialog : public KDialog
{
    Q_OBJECT
    
    public:
        CoverFoundDialog( QWidget *parent, const QList<QPixmap> &covers, const QString &productname );

        /**
        *   @returns the currently selected cover image
        */
        const QPixmap image() { return *m_labelPix->pixmap(); }

        virtual void accept()
        {
            if( qstrcmp( sender()->objectName().toAscii(), "NewSearch" ) == 0 )
                done( 1000 );
            else if( qstrcmp( sender()->objectName().toAscii(), "NextCover" ) == 0 )
                done( 1001 );
            else
                KDialog::accept();
        }

        private slots:
            /**
            *   Switch picture label and current index to next cover
            */
            void nextPix();

            /**
            *   Switch picture label and current index to previous cover
            */
            void prevPix();

        private:

            QLabel      *m_labelPix;        //! Picture Label
            QLabel      *m_labelName;       //! Name Label
            KHBox       *m_buttons;         //! Button Box
            KPushButton *m_next;            //! Next Button
            KPushButton *m_prev;            //! Back Button
            KPushButton *m_save;            //! Save Button
            KPushButton *m_cancel;          //! Cancel Button
            QList<QPixmap> m_covers;        //! Retrieved Covers
            int         m_curCover;         //! Currently selected Cover
};

namespace The
{
    inline CoverFetcher *coverFetcher() { return CoverFetcher::instance(); }
}

#endif /* AMAROK_COVERFETCHER_H */

/****************************************************************************************
 * Copyright (c) 2009-2010 Joffrey Clavel <jclavel@clabert.info>                        *
 * Copyright (c) 2009 Oleksandr Khayrullin <saniokh@gmail.com>                          *
 * Copyright (c) 2010 Alexandre Mendes <alex.mendes1988@gmail.com>                      *
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

#ifndef SIMILAR_ARTISTS_APPLET_H
#define SIMILAR_ARTISTS_APPLET_H


#include "ArtistWidget.h"
#include "SimilarArtist.h"
#include "context/Applet.h"
#include "context/DataEngine.h"

#include <ui_similarArtistsSettings.h>

#include <QStack>

class KConfigDialog;
class QGraphicsLinearLayout;

namespace Plasma
{
class IconWidget;
}

/**
 * SimilarArtists will display similar artists from the Internet, relative to the current playing artist.
 * @author Joffrey Clavel
 * @author Oleksandr Khayrullin
 * @version 0.1
 */
class SimilarArtistsApplet : public Context::Applet
{
    Q_OBJECT

public:

    /**
     * SimilarArtistsApplet constructor
     * @param parent The widget parent
     * @param args   List of strings containing two entries: the service id
     *               and the applet id
     */
    SimilarArtistsApplet( QObject* parent, const QVariantList& args );

    /**
     * SimilarArtistsApplet destructor
     */
    ~SimilarArtistsApplet();

protected:
    void createConfigurationInterface( KConfigDialog *parent );

public Q_SLOTS:
    /**
     * Initialization of the applet's display, creation of the layout, scrolls
     */
    virtual void init();

    /**
     * Update the current artist and his similar artists
     */
    void dataUpdated( const QString &source, const Plasma::DataEngine::Data &data );

private Q_SLOTS:
    void goBackward();
    void goForward();
    void updateNavigationIcons();
    void queryArtist( const QString &name );
    void queryForCurrentTrack();

private:

    /**
     * Update the display of the artists according to the lists m_similars
     */
    void artistsUpdate();

    /**
     * This scrollArea contents the artists widgets
     */
    ArtistsListWidget *m_scroll;

    /**
     * The list of similar artists to display
     */
    SimilarArtist::List m_similars;

    /**
     * Artist which you want to see artists like
     */
    QString m_artist;

    QStack<QString> m_historyBack;
    QStack<QString> m_historyForward;
    Plasma::IconWidget *m_backwardIcon;
    Plasma::IconWidget *m_forwardIcon;
    Plasma::IconWidget *m_currentArtistIcon;
    Plasma::IconWidget *m_settingsIcon;
    QGraphicsLinearLayout *m_layout;
    Ui::similarArtistsSettings ui_Settings;

    /**
     * The max number artists
     */
    int m_maxArtists;

private Q_SLOTS:

    /**
     * Allows the connection to the lastfm's api
     */
    void connectSource( const QString &source );

    /**
     * Show the settings windows
     */
    void configure();
    void saveSettings();

    void showSimilarArtists( const QString &name );
    void showArtistBio( const QString &name );
};

AMAROK_EXPORT_APPLET( similarArtists, SimilarArtistsApplet )

#endif // SIMILARARTISTSAPPLET_H

/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef PROGRESSIVESEARCHWIDGET_H
#define PROGRESSIVESEARCHWIDGET_H

#include <KVBox>
#include "LineEdit.h"

class KAction;

class QKeyEvent;
class QLabel;
class QMenu;

namespace Playlist
{

/**
    A composite widget for progressive (Firefox style search as you type)
    searching, with buttons for next and previous result. Also includes
    a drop down menu to configure which fields in the track ( track name,
    album name, genre, ...) should be used for matching, as well as a config
    option for whether the current search and search fields should be taken
    into account when doing track progression (i.e. should navigators only
    jump between tracks that match the current search term)

    @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class ProgressiveSearchWidget : public KVBox
{
    Q_OBJECT

public:
    /**
     * Constructor.
     * @param parent The parent widget this is added to.
     */
    ProgressiveSearchWidget( QWidget * parent );

    /**
     * Destructor.
     */
    ~ProgressiveSearchWidget();

    QString currentFilter() { return m_searchEdit->text(); }

    void setCurrentFilter( const QString filterExpr ) { m_searchEdit->setText( filterExpr ); }

    bool onlyMatches() { return m_showOnlyMatches; }

signals:
    /**
     * Signal emitted when the search term has changed.
     * @param filter The new search term.
     * @param filelds The mask containing the fields to match against.
     */
    void filterChanged( const QString &filter, int fields, bool showOnlyMatches );

    /**
     * Signal emitted when the search term is cleared.
     */
    void filterCleared();

    /**
     * Signal emitted when the "next" button is pressed.
     * @param filter The current search term.
     * @param filelds The mask containing the fields to match against.
     */
    void next( const QString &filter, int fields  );

    /**
     * Signal emitted when the "previous" button is pressed.
     * @param filter The current search term.
     * @param filelds The mask containing the fields to match against.
     */
    void previous( const QString &filter, int fields  );

    /**
     * Signal emitted when the user changes the value of the "Play only
     * matches" option.
     * @param showOnlyMatches The value selected by the user.
     */
    void showOnlyMatches( bool onlyMatches );

    /**
     * Signal emitted when the user presses the return key, signifying that the matched
     * item in the playlist should be activated
     */
    void activateFilterResult();

    /**
     * Signal emitted when the down key is pressed. Forwarded on from Amarok::LineEdit
     */
    void downPressed();

public slots:
    /**
     * Notify the widget that there are matches (at least one), so the next and previous actions
     * should be enabled and the text color set to normal.
     */
    void match();

    /**
     * Notify the widget that there are no matches, so the next and previous actions
     * should be disabled and the text color set to no_match color.
     */
    void noMatch();

    /**
     * Clear the filter..
     */
    void slotFilterClear();

    /**
     * Toggle navigate only tracks that match the current search term and
     * search fields. (The user can always manually select a track that
     * is not a part of the search results.
     * @param showOnlyMatches On/off.
     */
    void slotShowOnlyMatches( bool onlyMatches );

protected slots:
    /**
     * Notify widget that the text in the search edit has changed.
     * @param filter The new text in the search widget.
     */
    void slotFilterChanged( const QString &filter );

    /**
     * Notify widget that the "next" button has been pressed.
     */
    void slotNext();

    /**
     * Notify widget that the "previous" button has been pressed.
     */
    void slotPrevious();

    /**
     * Toggle track name matching when searching.
     * @param search On/off.
     */
    void slotSearchTracks( bool search );

    /**
     * Toggle artist name matching when searching.
     * @param search On/off.
     */
    void slotSearchArtists( bool search );

    /**
     * Toggle album name matching when searching.
     * @param search On/off.
     */
    void slotSearchAlbums( bool search );

    /**
     * Toggle genre name matching when searching.
     * @param search On/off.
     */
    void slotSearchGenre( bool search );

    /**
     * Toggle composer name matching when searching.
     * @param search On/off.
     */
    void slotSearchComposers( bool search );

    /**
     * Toggle year matching when searching.
     * @param search On/off.
     */
    void slotSearchYears( bool search );

protected:
    void keyPressEvent( QKeyEvent *event );

private:
    /**
     * Load the current search field settings from config.
     */
    void readConfig();

    void hideHiddenTracksWarning();
    void showHiddenTracksWarning();

    Amarok::LineEdit *m_searchEdit;
    KAction   *m_nextAction;
    KAction   *m_previousAction;
    QMenu     *m_menu;
    QLabel    *m_warningLabel;

    int        m_searchFieldsMask;
    bool       m_showOnlyMatches;

    QString    m_lastFilter;
};

}   //namespace Playlist

#endif

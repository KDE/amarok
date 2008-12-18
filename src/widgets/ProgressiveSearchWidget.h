/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
 
#ifndef PROGRESSIVESEARCHWIDGET_H
#define PROGRESSIVESEARCHWIDGET_H

#include <KHBox>

class KAction;
class KLineEdit;
class QMenu;

/**
A composite widget for progressive (Firefix style search as you type) searching, with buttons next and previous result

	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com> 
*/
class ProgressiveSearchWidget : public KHBox
{
    Q_OBJECT
public:
    ProgressiveSearchWidget( QWidget * parent );

    ~ProgressiveSearchWidget();

signals:

    /**
     * Signal emitted when the search term has changed
     * @param filter the new search term
     * @param filelds The mask containing the fields to match against
     */
    void filterChanged( const QString &filter, int fields );
    
    /**
     * Signal emitted when the search term is cleared
     */
    void filterCleared();
    
    /**
     * Signal emitted when the "next" button is pressed 
     * @param filter The current search term
     * @param filelds The mask containing the fields to match against
     */
    void next( const QString &filter, int fields  );
    
    /**
     * Signal emitted when the "previous" button is pressed
     * @param filter The current search term
     * @param filelds The mask containing the fields to match against
     */
    void previous( const QString &filter, int fields  );

public slots:

    /**
     * Notify the widget that there are matches (at least one), so the next and previous actions
     * should be enabled and the text color set to normal
     */
    void match();

    /**
     * Notify the widget that there are no matches, so the next and previous actions
     * should be disabled and the text color set to no_match color.
     */
    void noMatch();

protected slots:

    /**
     * Notify widget that the text in the search edit has changed
     * @param filter The new text in the search widget
     */
    void slotFilterChanged( const QString &filter );
    
    /**
     * Notify widget that the "next" button has been pressed
     */
    void slotNext();

    /**
     * Notify widget that the "previous" button has been pressed
     */
    void slotPrevious();

    void slotSearchTracks( bool search );
    void slotSearchArtists( bool search );
    void slotSearchAlbums( bool search );
    void slotSearchGenre( bool search );
    void slotSearchComposers( bool search );
    void slotSearchYears( bool search );
    
private:

    KLineEdit * m_searchEdit;
    KAction * m_nextAction;
    KAction * m_previousAction;
    QMenu * m_menu;

    int m_searchFieldsMask;
};

#endif

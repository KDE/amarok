/***************************************************************************
 * copyright            : (C) 2005 Seb Ruiz <me@sebruiz.net>               *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/***************************************************************************
 * Infiltrations of Party Mode                                             *
 *   Party mode is a complex playlist handling mechanism - acting          *
 *   basically on the concept of a 'rotating' playlist.  The playlist can  *
 *   be modelled as a queuing system, FIFO.  As a track is advanced,       *
 *   the first track in the playlist is removed, and another appended to   *
 *   the end.  The type of addition is selected by the user during         *
 *   configuration.                                                        *
 *                                                                         *
 *   Due to the nature of this type of handling, the status of party-mode  *
 *   must be determined, as many function require alternate handling.      *
 *      Examples include:                                                  *
 *          - Context Menus                                                *
 *          - Double clicking on an item -> requires moving the item to    *
 *            front of the queue                                           *
 *          - Undo/Redo states, to reinit history items                    *
 *   Please be aware of these when working with party mode.                *
 ***************************************************************************/

#ifndef AMAROK_PARTY_H
#define AMAROK_PARTY_H

#include "playlist.h"

#include <qlistview.h>
#include <qvbox.h>          //baseclass

class QLabel;
class KAction;
class KActionCollection;
class PartyDialogBase;
class PartyEntry;

class Party : public QVBox
{
        Q_OBJECT
    friend class PlaylistBrowser;

    public:
        Party( QWidget *parent, const char *name = 0 );
       ~Party();

        enum    Mode{ RANDOM=0, SUGGESTION=1, CUSTOM=2 };

        void    loadConfig( PartyEntry *config = 0 ); // 0 -> load amarokrc config

        bool    isChecked();
        int     previousCount();
        int     upcomingCount();
        int     appendCount();
        int     appendType();
        bool    cycleTracks();
        bool    markHistory();
        QString customList();

        static  Party *instance() { return s_instance; }

    public slots:
        void    repopulate()      { Playlist::instance()->repopulate(); }

    private slots:
        void    applySettings();
        void    setDynamicMode( bool enable, bool showDialog = true );
        void    showConfig( bool show );

    private:
        enum    UpdateMe{ PARTY, CYCLE, HISTORY, PREVIOUS, UPCOMING, APPEND, TYPE };

        PartyDialogBase *m_base;

        void    blockSignals( const bool b );
        void    createWidget();

        KActionCollection *m_ac;
        KAction    *m_repopulate;
        KToolBar   *m_toolbar;
        QLabel     *m_tooltip;
        bool        m_visible;

        QPtrList<QListViewItem> m_selected;


        static Party *s_instance;
};

#endif //AMAROK_PARTY_H

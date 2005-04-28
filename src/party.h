/***************************************************************************
 * copyright            : (C) 2005 Seb Ruiz <seb100@optusnet.com.au>       *
****************************************************************************/

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

#include <qcheckbox.h>

#include <kdialogbase.h>    //baseclass
#include <knuminput.h>      //kintspinbox

class QButtonGroup;
class QHBox;
class QListBox;
class QRadioButton;
class QVGroupBox;

class KActionSelector;


class Party : public KDialogBase
{
        Q_OBJECT

    public:
        Party( QString defaultName, QWidget *parent, const char *name = 0 );

        QString appendType();
        bool    isChecked()     { return m_partyCheck->isChecked(); }
        int     previousCount() { return m_previousIntSpinBox->value(); }
        int     upcomingCount() { return m_upcomingIntSpinBox->value(); }
        int     appendCount()   { return m_tracksToAddSpinBox->value(); }
        bool    cycleTracks()   { return m_cycleTracks->isChecked(); }

        QString customList();

    public slots:
        void    showHelp();

    private:
        void insertAvailablePlaylists();
        void insertSelectedPlaylists();

        void applySettings();

        QVGroupBox   *m_partyGroupBox;

        QButtonGroup *m_buttonGroup;
        QRadioButton *m_randomRadio;
        QRadioButton *m_suggestionRadio;
        QRadioButton *m_playlistRadio;
        KActionSelector *m_playlistSelector;
        QCheckBox    *m_partyCheck;
        QCheckBox    *m_cycleTracks;
        KIntSpinBox  *m_previousIntSpinBox;
        KIntSpinBox  *m_upcomingIntSpinBox;
        KIntSpinBox  *m_tracksToAddSpinBox;

        QLabel *m_tooltip;

        //List boxes for the KActionSelector
        QListBox     *m_lbSelected;
        QListBox     *m_lbAvailable;
};

#endif //AMAROK_PARTY_H

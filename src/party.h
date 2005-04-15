/***************************************************************************
  copyright            : (C) Seb Ruiz <seb100@optusnet.com.au>
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
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
        QString customList();

    private:
        void insertAvailablePlaylists();
        void insertSelectedPlaylists();

        QButtonGroup *m_buttonGroup;
        QRadioButton *m_randomRadio;
        QRadioButton *m_suggestionRadio;
        QRadioButton *m_playlistRadio;
        KActionSelector *m_playlistSelector;
        QCheckBox    *m_partyCheck;
        KIntSpinBox  *m_previousIntSpinBox;
        KIntSpinBox  *m_upcomingIntSpinBox;

        QListBox     *m_lbSelected;
        QListBox     *m_lbAvailable;

        KDialogBase  *m_genreSelector;
};

#endif //AMAROK_PARTY_H

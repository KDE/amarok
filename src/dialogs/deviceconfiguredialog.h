/****************************************************************************************
 * Copyright (c) 2005 Martin Aumueller <aumuell@reserv.at>                              *
 * Copyright (c) 2006 Jeff Mitchell <kde-dev@emailgoeshere.com>                         *
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

#ifndef DEVICECONFIGUREDIALOG_H
#define DEVICECONFIGUREDIALOG_H

#include "hintlineedit.h"
#include "MediaDevice.h"

#include <kdialog.h>

#include <QCheckBox>
#include <QRadioButton>


class Medium;

/**
    @author Jeff Mitchell <kde-dev@emailgoeshere.com>
*/
class DeviceConfigureDialog : public KDialog
{
    Q_OBJECT

    public:
        DeviceConfigureDialog( MediaDevice *device );
        ~DeviceConfigureDialog();
        bool successful() { return m_accepted; }

    private slots:
        void slotButtonClicked( KDialog::ButtonCode button );

    private:
        bool            m_accepted;
        MediaDevice     *m_device;

        HintLineEdit    *m_connectEdit;
        HintLineEdit    *m_disconnectEdit;
        QCheckBox       *m_transcodeCheck;
        QRadioButton    *m_transcodeAlways;
        QRadioButton    *m_transcodeWhenNecessary;
        QCheckBox       *m_transcodeRemove;

};

#endif

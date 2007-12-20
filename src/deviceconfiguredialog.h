/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
//
// C++ Interface: deviceconfiguredialog.h
//
// Description:
//
//
// Author: Jeff Mitchell <kde-dev@emailgoeshere.com>, (C) 2006
//         Martin Aumueller <aumuell@reserv.at>, (C) 2005
//
//
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

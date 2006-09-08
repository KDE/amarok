//
// C++ Interface: deviceconfiguredialog.h
//
// Description:
//
//
// Author: Jeff Mitchell <kde-dev@emailgoeshere.com>, (C) 2006
//         Martin Aumueller <aumuell@reserv.at>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef DEVICECONFIGUREDIALOG_H
#define DEVICECONFIGUREDIALOG_H

#include "hintlineedit.h"

#include <qcheckbox.h>
#include <qradiobutton.h>

#include <kdialogbase.h>

class MediaBrowser;
class Medium;

/**
    @author Jeff Mitchell <kde-dev@emailgoeshere.com>
*/
class DeviceConfigureDialog : public KDialogBase
{
    Q_OBJECT

    public:
        DeviceConfigureDialog( const Medium &medium );
        ~DeviceConfigureDialog();
        bool successful() { return m_accepted; };

    private slots:
        void slotOk();
        void slotCancel();

    private:
        bool            m_accepted;
        Medium*         m_medium;

        HintLineEdit    *m_connectEdit;
        HintLineEdit    *m_disconnectEdit;
        QCheckBox       *m_transcodeCheck;
        QRadioButton    *m_transcodeAlways;
        QRadioButton    *m_transcodeWhenNecessary;
        QCheckBox       *m_transcodeRemove;

};

#endif

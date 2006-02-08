//
// C++ Interface: mediumpluginchooser
//
// Description:
//
//
// Author: Jeff Mitchell <kde-dev@emailgoeshere.com>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TRANSFERDIALOG_H
#define TRANSFERDIALOG_H

#include "mediabrowser.h"

#include <qmap.h>
#include <qsignalmapper.h>
#include <kdialogbase.h>

class KComboBox;
class TransferDialog;

/**
	@author Jeff Mitchell <kde-dev@emailgoeshere.com>
*/
class TransferDialog : public KDialogBase
{
    Q_OBJECT

    public:
        TransferDialog( MediaDevice* mdev );
        ~TransferDialog() {}

        bool isAccepted() { return m_accepted; }

    signals:
        void selectedPlugin( const Medium*, const QString );

    private slots:
        void slotOk();
        void slotCancel();

    private:
        bool    m_accepted;

};

#endif


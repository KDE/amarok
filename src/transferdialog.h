/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
//
// C++ Interface: transferdialog
//
// Description:
//
//
// Author: Jeff Mitchell <kde-dev@emailgoeshere.com>, (C) 2005
//
#ifndef TRANSFERDIALOG_H
#define TRANSFERDIALOG_H

#include "mediabrowser.h"
#include "amarok_export.h"

#include <QCheckBox>
#include <qmap.h>
#include <QSignalMapper>
#include <QLabel>
#include <kdialog.h>

class KComboBox;
class TransferDialog;

/**
	@author Jeff Mitchell <kde-dev@emailgoeshere.com>
*/
class AMAROK_EXPORT TransferDialog : public KDialog
{
    Q_OBJECT

    public:
        TransferDialog( MediaDevice* mdev );
        ~TransferDialog() {}

        bool isAccepted() { return m_accepted; }

    private slots:
        void slotOk();
        void slotCancel();
        void sort1_activated( int index );
        void sort2_activated( int index );
        void convertSpaces_toggled( bool on );

    private:
        MediaDevice                     *m_dev;
        bool                             m_accepted;
        KComboBox                       *m_sort1;
        KComboBox                       *m_sort2;
        KComboBox                       *m_sort3;
        QLabel                          *m_label1;
        QLabel                          *m_label2;
        QLabel                          *m_label3;
        QList<KComboBox *>             m_combolist;
        int                              m_sort1LastIndex;
        int                              m_sort2LastIndex;
};

#endif


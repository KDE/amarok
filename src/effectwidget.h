/***************************************************************************
                          effectwidget.h  -  description
                             -------------------
    begin                : Mar 6 2003
    copyright            : (C) 2003 by Mark Kretschmann
    email                : markey@web.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef EFFECTWIDGET_H
#define EFFECTWIDGET_H

#include <kdialogbase.h>
#include <klistview.h>

class QWidget;
class KComboBox;

/**
 *@author mark
 */

//singleton
class EffectWidget : public KDialogBase
{
    Q_OBJECT

    public:
        EffectWidget( QWidget* parent = 0 );
        ~EffectWidget();

        static EffectWidget* self;

    private slots:
        void slotAdd();
        void slotRemove();
        void slotConfigure();
        void slotChanged( QListViewItem* );

    private:
// ATTRIBUTES ------
        static const int BUTTON_WIDTH = 30;
        static QRect         saveGeometry;

        KComboBox   *m_pComboBox;
        KListView   *m_pListView;
        QWidget     *m_pConfigureButton;
};

////////////////////////////////////////////////////////////////////////////////
// CLASS EffectListItem
////////////////////////////////////////////////////////////////////////////////

class EffectListItem : public KListViewItem
{
    public:
        EffectListItem( KListView *parent, const QString &label, long id = -1 );

        void configure();

// ATTRIBUTES ------
        long m_id;
};


#endif

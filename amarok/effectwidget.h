/***************************************************************************
                          effectwidget.h  -  description
                             -------------------
    begin                : Don Mär 6 2003
    copyright            : (C) 2003 by Mark Kretschmann
    email                :
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

#include <qlistview.h>
#include <qwidget.h>

#include <kdialogbase.h>

#include <arts/artsgui.h>

class QGroupBox;
class QPushButton;

class KComboBox;
class KArtsWidget;

namespace Arts
{
    class StereoEffect;
};

class PlayerApp;
extern PlayerApp *pApp;

/**
 *@author mark
 */

// CLASS EffectListItem --------------------------------------------------------

class EffectListItem : public QListViewItem
{
    public:
        EffectListItem( QListView *parent, const QString &label );
        ~EffectListItem();

        void configure();
        bool configurable() const;

// ATTRIBUTES ------
        long m_ID;
        Arts::StereoEffect *m_pFX;

    private:
};

// CLASS ArtsConfigWidget --------------------------------------------------------

class ArtsConfigWidget : public QWidget
{
    Q_OBJECT

    public:
        ArtsConfigWidget( Arts::Object object, QWidget *parent );
        ~ArtsConfigWidget();

    private:
        Arts::Widget m_gui;
        KArtsWidget *m_pArtsWidget;
};


// CLASS EffectWidget ----------------------------------------------------------

class EffectWidget : public KDialogBase
{
    Q_OBJECT

    public:
        EffectWidget( QWidget *parent = 0, const char *name = 0 );
        ~EffectWidget();

    public slots:
        void slotButtonTop();
        void slotButtonBotConf();
        void slotButtonBotRem();
        void slotItemClicked( QListViewItem *pCurrentItem );

    private:
        QStrList queryEffects() const;

// ATTRIBUTES ------
        KComboBox   *m_pComboBox;
        QListView *m_pListView;

        QPushButton *m_pButtonTopDown;
        QPushButton *m_pButtonBotConf;
        QPushButton *m_pButtonBotRem;

        QGroupBox *m_pGroupBoxTop;
        QGroupBox *m_pGroupBoxBot;

        Arts::StereoEffect *FX;
};
#endif

//
// C++ Interface: mediumpluginmanager
//
// Description:
//
//
// Author: Jeff Mitchell <kde-dev@emailgoeshere.com>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef MEDIUMPLUGINMANAGER_H
#define MEDIUMPLUGINMANAGER_H

#include "amarok.h"
#include "plugin/pluginconfig.h"
#include "pluginmanager.h"

#include <qlayout.h>
#include <qmap.h>

#include <kconfig.h>
#include <kdialogbase.h>
#include <klocale.h>

class QGroupBox;
class QLabel;
class QSignalMapper;
class QVBox;
class KComboBox;
class KLineEdit;
class Medium;
class MediumPluginManager;
class MediumPluginDetailView;

typedef QMap<Medium*, KComboBox*> ComboMap;
typedef QMap<int, Medium*> ButtonMap;
typedef QMap<int, QHBox*> HBoxMap;
typedef QMap<QString, Medium*> DeletedList;

/**
	@author Jeff Mitchell <kde-dev@emailgoeshere.com>
*/
class MediumPluginManager : public KDialogBase
{
    Q_OBJECT

    public:
        MediumPluginManager();
        ~MediumPluginManager();

    signals:
        void selectedPlugin( const Medium*, const QString );

    private slots:
        void slotOk();
        void infoRequested( int buttonId );
        void deleteMedium( int buttonId );
        void reDetectDevices();
        void newDevice();

    private:

        void detectDevices();

        ComboMap m_cmap;
        ButtonMap m_bmap;
        HBoxMap m_hmap;
        QSignalMapper *m_siginfomap;
        QSignalMapper *m_sigdelmap;
        DeletedList m_deletedList;

        QVBox *m_devicesBox;
        KConfig *m_config;
        QHBox *m_hbox;
        QString *m_currtext;
        QLabel *m_currlabel;
        QGroupBox *m_location;
        KComboBox *m_currcombo;
        KPushButton *m_currbutton;
        KPushButton *m_deletebutton;
        int m_buttonnum;
        bool m_redetect;

        KTrader::OfferList m_offers;
        KTrader::OfferList::ConstIterator m_offersEnd;
        KTrader::OfferList::ConstIterator m_plugit;
};

class MediumPluginDetailView : public KDialogBase
{
    Q_OBJECT

    public:
        MediumPluginDetailView( const Medium* medium );
        ~MediumPluginDetailView();
};

class ManualDeviceAdder : public KDialogBase
{
    Q_OBJECT

    public:
        ManualDeviceAdder();
        ~ManualDeviceAdder();
        Medium* getMedium();

    private:
        KComboBox* m_mdaCombo;
        KLineEdit* m_mdaName;
        KLineEdit* m_mdaMountPoint;

        KTrader::OfferList m_mdaOffers;
        KTrader::OfferList::ConstIterator m_mdaOffersEnd;
        KTrader::OfferList::ConstIterator m_mdaPlugit;
};

#endif


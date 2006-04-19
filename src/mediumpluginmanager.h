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
#include "hintlineedit.h"
#include "plugin/pluginconfig.h"

#include <qlayout.h>
#include <qmap.h>
#include <qhbox.h>

#include <kconfig.h>
#include <kdialogbase.h>
#include <klocale.h>

class QButton;
class QGroupBox;
class QLabel;
class QSignalMapper;
class QVBox;
class KComboBox;
class KLineEdit;
class Medium;
class MediumPluginManager;

typedef QMap<QString, Medium*> DeletedMap;
typedef QMap<QString, Medium*> NewDeviceMap;

/**
	@author Jeff Mitchell <kde-dev@emailgoeshere.com>
*/

class MediaDeviceConfig : public QHBox
{
    Q_OBJECT

    public:
        MediaDeviceConfig( Medium *medium, QWidget *parent=0, const char *name=0 );
        ~MediaDeviceConfig();
        QString oldPlugin();
        QString plugin();
        QButton *configButton();
        QButton *removeButton();
        Medium *medium();
        bool isNew();

    public slots:
        void configureDevice();
        void deleteDevice();

    signals:
        void deleteMedium( Medium *medium );

    protected:
        Medium *m_medium;
        QString m_oldPlugin;
        KComboBox * m_pluginCombo;
        QButton *m_configButton;
        QButton *m_removeButton;
        bool m_new;
};

typedef QValueList<MediaDeviceConfig *> DeviceList;

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
        void redetectDevices();
        void newDevice();
        void deleteMedium( Medium *medium );

    private:

        bool detectDevices( bool redetect=false );
        DeletedMap m_deletedMap;
        NewDeviceMap m_newDevMap;

        QVBox *m_devicesBox;
        QGroupBox *m_location;
        DeviceList m_deviceList;
};

class ManualDeviceAdder : public KDialogBase
{
    Q_OBJECT

    public:
        ManualDeviceAdder( MediumPluginManager* mdm );
        ~ManualDeviceAdder();
        bool successful() const { return m_successful; }
        Medium* getMedium();
        QString getPlugin() const { return m_selectedPlugin; }

    private slots:
        void slotCancel();
        void slotOk();
        void comboChanged( const QString & );

    private:
        MediumPluginManager* m_mpm;
        bool m_successful;
        QString m_comboOldText;
        QString m_selectedPlugin;

        KComboBox* m_mdaCombo;
        HintLineEdit* m_mdaName;
        HintLineEdit* m_mdaMountPoint;
};

#endif


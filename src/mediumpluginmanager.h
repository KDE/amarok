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

/**
    @author Jeff Mitchell <kde-dev@emailgoeshere.com>
    @author Martin Aumueller <aumuell@reserv.at>
*/

class MediaDeviceConfig : public QHBox
{
    Q_OBJECT

    public:
        MediaDeviceConfig( Medium *medium, MediumPluginManager *mgr, const bool nographics=false, QWidget *parent=0, const char *name=0 );
        ~MediaDeviceConfig();
        QString oldPlugin();
        void setOldPlugin( const QString &oldPlugin );
        QString plugin();
        KComboBox *pluginCombo();
        QButton *configButton();
        QButton *removeButton();
        Medium *medium();
        bool isNew();

    public slots:
        void configureDevice();
        void deleteDevice();

    signals:
        void deleteMedium( Medium *medium );
        void changed();

    protected:
        MediumPluginManager *m_manager;
        Medium *m_medium;
        QString m_oldPlugin;
        KComboBox * m_pluginCombo;
        QButton *m_configButton;
        QButton *m_removeButton;
        bool m_new;
};

typedef QValueList<MediaDeviceConfig *> DeviceList;

class MediumPluginManager : public QObject
{
    Q_OBJECT

    friend class DeviceManager;

    public:
        //nographics only for the initial run of detectDevices...pass in
        //directly to detectDevices after
        MediumPluginManager( QWidget *widget, const bool nographics=false );
        ~MediumPluginManager();
        void finished();
        bool hasChanged();

    signals:
        void selectedPlugin( const Medium*, const QString );
        void changed();

    public slots:
        void redetectDevices();
        void newDevice();
        void deleteMedium( Medium *medium );
        void slotChanged();

    private:
        bool detectDevices( bool redetect=false, bool nographics=false );
        DeletedMap m_deletedMap;
        DeviceList m_deviceList;
        QWidget *m_widget;
        bool m_hasChanged;

};

class MediumPluginManagerDialog : public KDialogBase
{
    Q_OBJECT

    public:
        MediumPluginManagerDialog();
        ~MediumPluginManagerDialog();

    private slots:
        void slotOk();

    private:

        QVBox *m_devicesBox;
        QGroupBox *m_location;
        MediumPluginManager *m_manager;
};

class ManualDeviceAdder : public KDialogBase
{
    Q_OBJECT

    public:
        ManualDeviceAdder( MediumPluginManager* mdm );
        ~ManualDeviceAdder();
        bool successful() const { return m_successful; }
        Medium* getMedium( bool recreate = false );
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
        Medium *m_newMed;

        KComboBox* m_mdaCombo;
        HintLineEdit* m_mdaName;
        HintLineEdit* m_mdaMountPoint;
};

#endif


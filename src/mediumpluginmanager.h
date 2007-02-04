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

#include <QLayout>
#include <qmap.h>
#include <q3hbox.h>
//Added by qt3to4:
#include <QLabel>
#include <Q3ValueList>

#include <kconfig.h>
#include <kpagedialog.h>
#include <klocale.h>

class QAbstractButton;
class Q3GroupBox;
class QLabel;
class QSignalMapper;
class Q3VBox;
class KComboBox;
class KLineEdit;
class Medium;
class MediumPluginManager;

typedef QMap<QString, Medium*> DeletedMap;

/**
    @author Jeff Mitchell <kde-dev@emailgoeshere.com>
    @author Martin Aumueller <aumuell@reserv.at>
*/

class MediaDeviceConfig : public Q3HBox
{
    Q_OBJECT

    public:
        MediaDeviceConfig( Medium *medium, MediumPluginManager *mgr, const bool nographics=false, QWidget *parent=0, const char *name=0 );
        ~MediaDeviceConfig();
        QString oldPlugin();
        void setOldPlugin( const QString &oldPlugin );
        QString plugin();
        KComboBox *pluginCombo();
        QAbstractButton *configButton();
        QAbstractButton *removeButton();
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
        QAbstractButton *m_configButton;
        QAbstractButton *m_removeButton;
        bool m_new;
};

typedef Q3ValueList<MediaDeviceConfig *> DeviceList;

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

class MediumPluginManagerDialog : public KPageDialog
{
    Q_OBJECT

    public:
        MediumPluginManagerDialog();
        ~MediumPluginManagerDialog();

    private slots:
        void slotOk();

    private:

        Q3VBox *m_devicesBox;
        Q3GroupBox *m_location;
        MediumPluginManager *m_manager;
};

class ManualDeviceAdder : public KPageDialog
{
    Q_OBJECT

    public:
        ManualDeviceAdder( MediumPluginManager* mdm );
        ~ManualDeviceAdder();
        bool successful() const { return m_successful; }
        Medium* getMedium( bool recreate = false );
        QString getPlugin() const { return m_selectedPlugin; }

    private slots:
        void slotButtonClicked( KDialog::ButtonCode);
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


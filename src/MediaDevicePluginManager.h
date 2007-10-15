//
// C++ Interface: mediadevicepluginmanager
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


#include <KHBox>
#include <KConfig>
#include <KPageDialog>
#include <KLocale>

#include <QLayout>
#include <QMap>
#include <QLabel>
#include <Q3ValueList>

class QAbstractButton;
class Q3GroupBox;
class QLabel;
class KVBox;
class KComboBox;
class Medium;
class MediaDevicePluginManager;

typedef QMap<QString, bool> DeletedMap;

/**
    @author Jeff Mitchell <kde-dev@emailgoeshere.com>
    @author Martin Aumueller <aumuell@reserv.at>
*/

class MediaDeviceConfig : public KHBox
{
    Q_OBJECT

    public:
        MediaDeviceConfig( QString id, MediaDevicePluginManager *mgr, const bool nographics=false, QWidget *parent=0, const char *name=0 );
        ~MediaDeviceConfig();
        QString oldPlugin() { return m_oldPlugin; }
        void setOldPlugin( const QString &oldPlugin ) { m_oldPlugin = oldPlugin; }
        inline QString plugin() { return MediaDevicePluginManager::instance()->getInternalPluginName( m_pluginCombo->currentText() ); }
        KComboBox *pluginCombo() { return m_pluginCombo; }
        QAbstractButton *configButton() { return m_configButton; }
        QAbstractButton *removeButton() { return m_removeButton; }
        QString uid() { return m_uid; }
        bool isNew() { return m_new; }

    public slots:
        void configureDevice();
        void deleteDevice();

    signals:
        void changed();

    protected:
        MediaDevicePluginManager *m_manager;
        QString m_uid;
        QString m_oldPlugin;
        KComboBox * m_pluginCombo;
        QAbstractButton *m_configButton;
        QAbstractButton *m_removeButton;
        bool m_new;
};

typedef QList<MediaDeviceConfig *> DeviceList;

class MediaDevicePluginManager : public QObject
{
    Q_OBJECT

    public:
        //nographics only for the initial run of detectDevices...pass in
        //directly to detectDevices after
        explicit MediaDevicePluginManager( QWidget *widget, const bool nographics=false );
        ~MediaDevicePluginManager();
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

class MediaDevicePluginManagerDialog : public KPageDialog
{
    Q_OBJECT

    public:
        MediaDevicePluginManagerDialog();
        ~MediaDevicePluginManagerDialog();

    private slots:
        void slotOk();

    private:

        KVBox *m_devicesBox;
        Q3GroupBox *m_location;
        MediaDevicePluginManager *m_manager;
};

class ManualDeviceAdder : public KPageDialog
{
    Q_OBJECT

    public:
        ManualDeviceAdder( MediaDevicePluginManager* mdm );
        ~ManualDeviceAdder();
        bool successful() const { return m_successful; }
        QString getId( bool recreate = false );
        QString getPlugin() const { return m_selectedPlugin; }

    private slots:
        void slotButtonClicked( KDialog::ButtonCode);
        void comboChanged( const QString & );

    private:
        MediaDevicePluginManager* m_mpm;
        bool m_successful;
        QString m_comboOldText;
        QString m_selectedPlugin;
        QString m_newId;

        KComboBox* m_mdaCombo;
        HintLineEdit* m_mdaName;
        HintLineEdit* m_mdaMountPoint;
};

#endif


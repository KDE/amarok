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
#include <KComboBox>
#include <KLocale>
#include <KDialog>

#include <QLayout>
#include <QMap>
#include <QLabel>
#include <Q3ValueList>

class QAbstractButton;
class QGroupBox;
class QLabel;
class KVBox;
class MediaDevice;
class MediaDevicePluginManager;

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
        QString plugin();
        KComboBox* pluginCombo() { return m_pluginCombo; }
        QAbstractButton *removeButton() { return m_removeButton; }
        QString udi() { return m_udi; }
        bool isNew() { return m_new; }

    public slots:
        void slotConfigureDevice();
        void slotDeleteDevice();
        void slotDetailsActivated( const QString & link );

    signals:
        void changed();
        void deleteDevice( const QString & );

    protected:
        MediaDevicePluginManager *m_manager;
        QString m_udi;
        QString m_name;
        QString m_oldPlugin;
        QString m_details;
        KComboBox* m_pluginCombo;
        QAbstractButton *m_removeButton;
        QLabel* m_label_details;
        bool m_new;
};

class MediaDevicePluginManager : public QObject
{
    Q_OBJECT

    public:
        //nographics only for the initial run of detectDevices...pass in
        //directly to detectDevices after
        explicit MediaDevicePluginManager( QWidget *widget, const bool nographics=false );
        ~MediaDevicePluginManager();
        void finished();

    signals:
        void selectedPlugin( const QString &, const QString & );

    public slots:
        void slotDeleteDevice( const QString &udi );
        void slotGenericVolumes();
        void slotNewDevice();
        void slotSolidDeviceAdded( const QString &udi );
        void slotSolidDeviceRemoved( const QString &udi );

    private:
        bool detectDevices( bool nographics=false );
        QMap<QString, MediaDeviceConfig*> m_deletedMap;
        QList<MediaDeviceConfig*> m_deviceList;
        QWidget *m_widget;

};

class MediaDevicePluginManagerDialog : public KDialog
{
    Q_OBJECT

    public:
        MediaDevicePluginManagerDialog();
        ~MediaDevicePluginManagerDialog();

    private slots:
        void slotOk();

    private:
        KPushButton* m_genericDevices;
        KPushButton* m_addButton;
        KVBox *m_devicesBox;
        QGroupBox *m_location;
        MediaDevicePluginManager *m_manager;
};

class ManualDeviceAdder : public KDialog
{
    Q_OBJECT

    public:
        ManualDeviceAdder( MediaDevicePluginManager* mdm );
        ~ManualDeviceAdder();
        bool successful() const { return m_successful; }
        QString getId( bool recreate = false );
        QString getPlugin() const { return m_selectedPlugin; }

    private slots:
        void slotButtonClicked( int button );
        void slotComboChanged( const QString & );

    private:
        MediaDevicePluginManager* m_mpm;
        bool m_successful;
        QString m_mountPointOldText;
        QString m_selectedPlugin;
        QString m_newId;

        KComboBox* m_mdaCombo;
        HintLineEdit* m_mdaName;
        HintLineEdit* m_mdaMountPoint;
};

class MediaDeviceVolumeMarkerDialog : public KDialog
{
    Q_OBJECT

    public:
        MediaDeviceVolumeMarkerDialog( MediaDevicePluginManager* mpm );
        ~MediaDeviceVolumeMarkerDialog();

    private slots:
        void slotOk();

    private:
        KVBox *m_mountPointBox;
        QGroupBox *m_location;
        MediaDevicePluginManager* m_mpm;
};

#endif


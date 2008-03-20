/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
//
// C++ Interface: mediadevicepluginmanager
//
// Description:
//
//
// Author: Jeff Mitchell <kde-dev@emailgoeshere.com>, (C) 2005
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
class QTableWidget;
class KVBox;
class MediaDevice;

/**
    @author Jeff Mitchell <kde-dev@emailgoeshere.com>
    @author Martin Aumueller <aumuell@reserv.at>
*/

class MediaDeviceConfig : public KHBox
{
    Q_OBJECT

    public:
        explicit MediaDeviceConfig( QString id, QWidget *parent=0, const char *name=0 );
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
        explicit MediaDevicePluginManager( QWidget *widget );
        ~MediaDevicePluginManager();
        void finished();

    signals:
        void selectedPlugin( const QString &, const QString & );

    public slots:
        void slotAddDevice( const QString &udi );
        void slotDeleteDevice( const QString &udi );
        void slotGenericVolumes();
        void slotNewDevice();
        void slotSolidDeviceAdded( const QString &udi );
        void slotSolidDeviceRemoved( const QString &udi );

    private:
        bool detectDevices();
        QStringList m_deletedList;
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
        ManualDeviceAdder();
        ~ManualDeviceAdder();
        bool successful() const { return m_successful; }
        QString getId( bool recreate = false );
        QString getPlugin() const { return m_selectedPlugin; }

    private slots:
        void slotButtonClicked( int button );
        void slotComboChanged( const QString & );

    private:
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
        MediaDeviceVolumeMarkerDialog();
        ~MediaDeviceVolumeMarkerDialog();
        const QStringList getAddedList() const { return m_addedList; }
        const QStringList getRemovedList() const { return m_removedList; }

    private slots:
        void slotOk();

    private:
        KVBox *m_mountPointBox;
        QStringList m_addedList;
        QStringList m_removedList;
        QTableWidget *m_table;
};

#endif


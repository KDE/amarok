/***************************************************************************
 *   Copyright (C) 2005 Paul Cifarelli <paul@cifarelli.net>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _HELIX_CONFIG_H_
#define _HELIX_CONFIG_H_

#include "plugin/pluginconfig.h"
#include <qptrlist.h>
#include <qtabwidget.h>
#include <qvbox.h>

class QGridLayout;
class KComboBox;
class QCheckBox;
class KLineEdit;
class HelixEngine;

// since many preferences can be set in Helix, I'm planning on more config items later
// for now I'll just get the location of the Helix core/plugins for initializing
// the Helix core
class HelixConfigEntry : public QObject
{
Q_OBJECT
public:
   HelixConfigEntry( QWidget *parent, Amarok::PluginConfig*,
                     int row, const QString & description, const char *defaultvalue, const QString & tooltip );
   HelixConfigEntry( QWidget *parent, QString &str, Amarok::PluginConfig*,
                     int row, const QString & description, const char *defaultvalue, const QString & tooltip );

   bool isChanged() const { return m_valueChanged; }
   void setUnchanged() { m_valueChanged = false; }
   const QString& key() const { return m_key; }
   QString stringValue() const { return m_stringValue; }
   int numValue() const { return m_numValue; }

private slots:
   void slotStringChanged( const QString& );

private:
   KLineEdit *m_w;
   bool       m_valueChanged;
   int        m_numValue;
   QString    m_key;
   QString    m_stringValue;
};

class HelixSoundDevice : public QObject
{
Q_OBJECT
public:
   HelixSoundDevice( QWidget *parent, Amarok::PluginConfig *config, int &row, HelixEngine *engine );
   bool save();
   void setSoundSystem( int api );
   bool isChanged() const { return m_changed; }
   void setUnchanged() { m_changed = false; }

private slots:
   void slotNewDevice( const QString& );
   void slotStringChanged( const QString& );
   void slotDeviceChecked( bool );

private:
   KComboBox* deviceComboBox;
   QCheckBox* checkBox_outputDevice;
   KLineEdit* lineEdit_outputDevice;
   bool m_changed;
   HelixEngine *m_engine;
};


class HelixConfigDialogBase : public QTabWidget
{
public:
   HelixConfigDialogBase( HelixEngine *engine, Amarok::PluginConfig *config, QWidget *parent = 0 );
   ~HelixConfigDialogBase();

   virtual QWidget *view() { return this; }
   virtual bool hasChanged() const;
   virtual bool isDefault() const;

   /** Save view state into configuration */
   virtual void save();

   void setSoundSystem( int api );
   void setEngine(HelixEngine *e) { m_engine = e; }

private:
   QPtrList<HelixConfigEntry> entries;
   HelixConfigEntry *m_core;
   HelixConfigEntry *m_plugin;
   HelixConfigEntry *m_codec;
   HelixSoundDevice *m_device;
   HelixEngine *m_engine;
};

class HelixConfigDialog : public Amarok::PluginConfig
{
public:
   HelixConfigDialog( HelixEngine *engine, QWidget *parent = 0 );
   ~HelixConfigDialog();

   virtual QWidget *view() { return instance->view(); }
   virtual bool hasChanged() const { return instance->hasChanged(); }
   virtual bool isDefault() const { return instance->isDefault(); }

   virtual void save() { instance->save(); }
   static int setSoundSystem( int api );

private:
   static HelixConfigDialogBase *instance;
};


#endif

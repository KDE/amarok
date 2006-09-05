/***************************************************************************
 *   Copyright (C) 2005 Paul Cifarelli <paul@cifarelli.net>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qscrollview.h>
#include <qspinbox.h>
#include <qtooltip.h>
#include <qtextedit.h>
#include <qtextview.h>
#include <qfileinfo.h>
#include <qcheckbox.h>

#include <klineedit.h>
#include <kseparator.h>
#include <klocale.h>
#include <kcombobox.h>

#include "helix-configdialog.h"
#include "helix-engine.h"

#include "config/helixconfig.h"
#include <config.h>

#include <iostream>

#define DEBUG_PREFIX "helix-engine"
#define indent helix_indent

#include "debug.h"

using namespace std;

HelixConfigDialogBase *HelixConfigDialog::instance = NULL;

HelixConfigEntry::HelixConfigEntry( QWidget *parent,
                                    Amarok::PluginConfig *pluginConfig,
                                    int row,
				    const QString & description,
                                    const char *defaultvalue,
                                    const QString & tooltip)
   :     m_w(0)
         , m_valueChanged( false )
         , m_stringValue( defaultvalue )
{
    QGridLayout *grid = (QGridLayout*)parent->layout();

    m_w = new KLineEdit( m_stringValue, parent );
    connect( (QWidget *) m_w, SIGNAL(textChanged( const QString& )), this, SLOT(slotStringChanged( const QString& )) );
    connect( (QWidget *) m_w, SIGNAL(textChanged( const QString& )), pluginConfig, SIGNAL(viewChanged()) );

    QToolTip::add( (QWidget *) m_w, "<qt>" + tooltip );

    QLabel* d = new QLabel( description + ':', parent );
    d->setAlignment( QLabel::WordBreak | QLabel::AlignVCenter );

    grid->addWidget( (QWidget *) m_w, row, 1 );
    grid->addWidget( d, row, 0 );
}

HelixConfigEntry::HelixConfigEntry( QWidget *parent,
                                    QString &str,
                                    Amarok::PluginConfig *pluginConfig,
                                    int row,
                                    const QString & description,
                                    const char *defaultvalue,
                                    const QString & tooltip)
   : m_w(0)
   , m_valueChanged( false )
   , m_stringValue( defaultvalue )
{
    QGridLayout *grid = (QGridLayout*)parent->layout();

    m_key = str;

    m_w = new KLineEdit( str, parent );
    connect( m_w, SIGNAL(textChanged( const QString& )), this, SLOT(slotStringChanged( const QString& )) );
    connect( m_w, SIGNAL(textChanged( const QString& )), pluginConfig, SIGNAL(viewChanged()) );

    QToolTip::add( m_w, "<qt>" + tooltip );

    QLabel* d = new QLabel( description + ':', parent );
    d->setAlignment( QLabel::WordBreak | QLabel::AlignVCenter );

    grid->addWidget( m_w, row, 1 );
    grid->addWidget( d, row, 0 );
}


inline void
HelixConfigEntry::slotStringChanged( const QString& )
{
   m_stringValue = m_w->text();
   m_valueChanged = true;
}

HelixSoundDevice::HelixSoundDevice( QWidget *parent,
                                    Amarok::PluginConfig *pluginConfig,
                                    int &row,
                                    HelixEngine *engine )
   : deviceComboBox(0), checkBox_outputDevice(0), lineEdit_outputDevice(0), m_changed(false), m_engine(engine)
{
   QGridLayout *grid = (QGridLayout*)parent->layout();

   deviceComboBox = new KComboBox( false, parent, "deviceComboBox" );
   deviceComboBox->insertItem("oss");  // I believe these are not subject to translation (they don't seem to be in xine,
#ifdef USE_HELIX_ALSA
   deviceComboBox->insertItem("alsa"); // and neither are the equivalents in gst (osssink and alsasink)
#endif
   deviceComboBox->setCurrentItem(HelixConfig::outputplugin());
   QLabel* op = new QLabel( i18n("Output plugin:"), parent );
   op->setAlignment( QLabel::WordBreak | QLabel::AlignVCenter );
   grid->addWidget( op, row, 0 );
   grid->addWidget( deviceComboBox, row, 1);
   connect( (QWidget *)deviceComboBox, SIGNAL( activated( const QString& ) ), this, SLOT( slotNewDevice( const QString& )) );
   connect( (QWidget *)deviceComboBox, SIGNAL( activated( const QString& )), pluginConfig, SIGNAL(viewChanged()) );

   ++row;

   checkBox_outputDevice = new QCheckBox( parent, "checkBox_outputDevice" );
   checkBox_outputDevice->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)5, (QSizePolicy::SizeType)5, 0, 0, checkBox_outputDevice->sizePolicy().hasHeightForWidth() ) );
   grid->addWidget( checkBox_outputDevice, row, 0 );
   checkBox_outputDevice->setText( i18n( "Device:" ) );

    lineEdit_outputDevice = new KLineEdit( HelixConfig::device(), parent );
    connect( (QWidget *) lineEdit_outputDevice, SIGNAL(textChanged( const QString& )), this, SLOT(slotStringChanged( const QString& )) );
    connect( (QWidget *) lineEdit_outputDevice, SIGNAL( textChanged( const QString& )), pluginConfig, SIGNAL(viewChanged()) );
    connect( checkBox_outputDevice, SIGNAL( toggled(bool) ), lineEdit_outputDevice, SLOT( setEnabled(bool) ) );
    connect( checkBox_outputDevice, SIGNAL( toggled(bool) ), pluginConfig, SIGNAL(viewChanged()) );

    connect( checkBox_outputDevice, SIGNAL( toggled(bool) ), this, SLOT( slotDeviceChecked(bool) ) );
    grid->addWidget( (QWidget *) lineEdit_outputDevice, row, 1 );

    if (HelixConfig::deviceenabled())
    {
       checkBox_outputDevice->setChecked( true );
       lineEdit_outputDevice->setEnabled( true );
    }
    else
    {
       checkBox_outputDevice->setChecked( false );
       lineEdit_outputDevice->setEnabled( false );
    }

    if (HelixConfig::outputplugin() == "oss")
    {
       checkBox_outputDevice->setEnabled( false );
       lineEdit_outputDevice->setEnabled( false );
    }
}

void
HelixSoundDevice::slotNewDevice( const QString &dev )
{
   if (dev == "oss")
   {
      checkBox_outputDevice->setEnabled( false );
      lineEdit_outputDevice->setEnabled(false);
   }
   else
   {
      checkBox_outputDevice->setEnabled( true );
      if (checkBox_outputDevice->isChecked())
         lineEdit_outputDevice->setEnabled( true );
      else
         lineEdit_outputDevice->setEnabled( false );
   }

   m_changed = true;
}

void
HelixSoundDevice::slotStringChanged( const QString& )
{
   m_changed = true;
}

void
HelixSoundDevice::slotDeviceChecked( bool checked )
{
   checkBox_outputDevice->setChecked( checked );
   if (checked)
      lineEdit_outputDevice->setEnabled( true );
   else
      lineEdit_outputDevice->setEnabled( false );
   m_changed = true;
}

bool
HelixSoundDevice::save()
{
   if (m_changed)
   {
      HelixConfig::setOutputplugin(deviceComboBox->currentText());
      if (deviceComboBox->currentText() == "oss")
         m_engine->setOutputSink(HelixSimplePlayer::OSS);
      else
         m_engine->setOutputSink(HelixSimplePlayer::ALSA);

      HelixConfig::setDevice( lineEdit_outputDevice->text() );
      if (checkBox_outputDevice->isChecked())
         m_engine->setDevice( lineEdit_outputDevice->text().utf8() );
      else
         m_engine->setDevice("default");
      HelixConfig::setDeviceenabled( checkBox_outputDevice->isChecked() );
   }

   return m_changed;
}

void HelixSoundDevice::setSoundSystem( int api )
{
   switch (api)
   {
      case HelixSimplePlayer::OSS:
         deviceComboBox->setCurrentItem("oss");
         checkBox_outputDevice->setEnabled( false );
         lineEdit_outputDevice->setEnabled(false);
         break;

      case HelixSimplePlayer::ALSA:
         deviceComboBox->setCurrentItem("alsa");
         checkBox_outputDevice->setEnabled( true );
         if (checkBox_outputDevice->isChecked())
            lineEdit_outputDevice->setEnabled( true );
         else
            lineEdit_outputDevice->setEnabled( false );
         break;
   };
   HelixConfig::setOutputplugin(deviceComboBox->currentText());
   HelixConfig::writeConfig();
}

void HelixConfigDialogBase::setSoundSystem( int api )
{
   m_device->setSoundSystem(api);
}


HelixConfigDialogBase::HelixConfigDialogBase( HelixEngine *engine, Amarok::PluginConfig *config, QWidget *p )
     : QTabWidget( p )
     , m_core(0)
     , m_plugin(0)
     , m_codec(0)
     , m_device(0)
     , m_engine( engine )
{
    int row = 0;
    QString currentPage;
    QWidget *parent = 0;
    QGridLayout *grid = 0;
    QScrollView *sv = 0;

    QString pageName( i18n("Main") );

    addTab( sv = new QScrollView, pageName );
    parent = new QWidget( sv->viewport() );

    sv->setResizePolicy( QScrollView::AutoOneFit );
    sv->setHScrollBarMode( QScrollView::AlwaysOff );
    sv->setFrameShape( QFrame::NoFrame );
    sv->addChild( parent );

    grid = new QGridLayout( parent, /*rows*/20, /*cols*/2, /*margin*/10, /*spacing*/10 );
    grid->setColStretch( 0, 1 );
    grid->setColStretch( 1, 1 );

    if( sv )
       sv->setMinimumWidth( grid->sizeHint().width() + 20 );

    engine->m_coredir = HelixConfig::coreDirectory();
    m_core = new HelixConfigEntry( parent, engine->m_coredir,
                                   config, row,
                                   i18n("Helix/Realplay core directory"),
                                   HelixConfig::coreDirectory().utf8(),
                                   i18n("This is the directory where clntcore.so is located"));
    ++row;
    engine->m_pluginsdir = HelixConfig::pluginDirectory();
    m_plugin = new HelixConfigEntry( parent, engine->m_pluginsdir,
                                     config, row,
                                     i18n("Helix/Realplay plugins directory"),
                                     HelixConfig::pluginDirectory().utf8(),
                                     i18n("This is the directory where, for example, vorbisrend.so is located"));
    ++row;
    engine->m_codecsdir = HelixConfig::codecsDirectory();
    m_codec = new HelixConfigEntry( parent, engine->m_codecsdir,
                                     config, row,
                                     i18n("Helix/Realplay codecs directory"),
                                     HelixConfig::codecsDirectory().utf8(),
                                     i18n("This is the directory where, for example, cvt1.so is located"));
    ++row;
    grid->addMultiCellWidget( new KSeparator( KSeparator::Horizontal, parent ), row, row, 0, 1 );

    ++row;
    m_device = new HelixSoundDevice( parent, config, row, engine );

    // lets find the logo if we can
    QPixmap *pm = 0;
    QString logo = HelixConfig::coreDirectory();
    if (logo.isEmpty())
       logo = HELIX_LIBS "/common";

    logo.append("/../share/");

    QString tmp = logo;
    tmp.append("hxplay/logo.png");
    if (QFileInfo(tmp).exists())
    {
       logo = tmp;
       pm = new QPixmap(logo);
    }
    else
    {
       tmp = logo;
       tmp.append("realplay/logo.png");
       if (QFileInfo(tmp).exists())
       {
          logo = tmp;
          pm = new QPixmap(logo);
       }
    }

    if (pm)
    {
       QLabel *l = new QLabel(parent);
       l->setPixmap(*pm);
       grid->addMultiCellWidget( l, 20, 20, 1, 1, Qt::AlignRight );
    }

    entries.setAutoDelete( true );

    pageName = i18n("Plugins");

    addTab( sv = new QScrollView, pageName );
    parent = new QWidget( sv->viewport() );

    sv->setResizePolicy( QScrollView::AutoOneFit );
    sv->addChild( parent );

    QTextEdit *le = new QTextEdit( parent );
    if( sv )
       sv->setMinimumWidth( le->sizeHint().width() );

    grid = new QGridLayout( parent, /*rows*/1, /*cols*/1, /*margin*/2, /*spacing*/1 );
    grid->addMultiCellWidget( le, 0, 1, 0, 1, 0 );
    le->setWordWrap(QTextEdit::NoWrap);

    int n = engine->numPlugins();
    const char *description, *copyright, *moreinfourl;
    row = 0;
    for (int i=0; i<n; i++)
    {
       if (!engine->getPluginInfo(i, description, copyright, moreinfourl))
       {
          le->append(QString(description));
          le->append(QString(copyright));
          le->append(QString(moreinfourl));
          le->append(QString(" "));
       }
    }

    le->setReadOnly(true);
    le->setContentsPos(0,0);
}

HelixConfigDialogBase::~HelixConfigDialogBase()
{
   delete m_core;
   delete m_plugin;
   delete m_codec;
   delete m_device;
}

bool
HelixConfigDialogBase::hasChanged() const
{
   for( QPtrListIterator<HelixConfigEntry> it( entries ); *it != 0; ++it )
      if ( (*it)->isChanged() )
         return true;
   if (m_core->isChanged() || m_plugin->isChanged() || m_codec->isChanged() || m_device->isChanged())
      return true;

   return false;
}

bool
HelixConfigDialogBase::isDefault() const
{
   return false;
}

void
HelixConfigDialogBase::save()
{
   bool writeIt = false;

   if (m_core->isChanged())
   {
      m_engine->m_coredir = m_core->stringValue();
      HelixConfig::setCoreDirectory(m_engine->m_coredir);
      writeIt = true;
   }

   if (m_plugin->isChanged())
   {
      m_engine->m_pluginsdir = m_plugin->stringValue();
      HelixConfig::setPluginDirectory(m_engine->m_pluginsdir);
      writeIt = true;
   }

   if (m_codec->isChanged())
   {
      m_engine->m_codecsdir = m_codec->stringValue();
      HelixConfig::setCodecsDirectory(m_engine->m_codecsdir);
      writeIt = true;
   }

   writeIt |= m_device->save();

   // not really doing anything here yet
   for( HelixConfigEntry *entry = entries.first(); entry; entry = entries.next() )
   {
      if( entry->isChanged() )
      {
         entry->setUnchanged();
      }
   }

   if (m_device->isChanged())
   {
      m_device->setUnchanged();
      writeIt = true;
   }

   if (writeIt)
   {
      HelixConfig::writeConfig();

      // reinit...
      m_engine->init();
   }

}

HelixConfigDialog::HelixConfigDialog( HelixEngine *engine, QWidget *p ) : Amarok::PluginConfig()
{
   if (!instance)
      instance = new HelixConfigDialogBase( engine, this, p );
}

HelixConfigDialog::~HelixConfigDialog()
{
   delete instance;
   instance = 0;
}

int HelixConfigDialog::setSoundSystem( int api )
{
   if (instance)
   {
      instance->setSoundSystem(api);
      return 0;
   }
   else
   {
      HelixConfig::setOutputplugin(api ? "alsa" : "oss");
      HelixConfig::writeConfig();
      return 1;
   }
}

#include "helix-configdialog.moc"

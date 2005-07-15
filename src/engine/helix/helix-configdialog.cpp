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

#include <klineedit.h>
#include <kseparator.h>
#include <klocale.h>

#include "helix-configdialog.h"
#include "helix-engine.h"

#include "config/helixconfig.h"

#include <iostream>

#define DEBUG_PREFIX "helix-engine"
#define indent helix_indent

#include "debug.h"

using namespace std;

HelixConfigEntry::HelixConfigEntry( QWidget *parent, 
                                    amaroK::PluginConfig *pluginConfig, 
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
                                    amaroK::PluginConfig *pluginConfig, 
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


HelixConfigDialog::HelixConfigDialog( HelixEngine *engine, QWidget *p )
   : amaroK::PluginConfig()
     , QTabWidget( p )
     , m_core(0)
     , m_plugin(0)
     , m_codec(0)
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
                                   this, row, 
                                   i18n("Helix/Realplay core directory"), 
                                   HelixConfig::coreDirectory().utf8(),
                                   i18n("This is the directory where clntcore.so is located"));
    ++row;
    engine->m_pluginsdir = HelixConfig::pluginDirectory();
    m_plugin = new HelixConfigEntry( parent, engine->m_pluginsdir, 
                                     this, row, 
                                     i18n("Helix/Realplay plugins directory"), 
                                     HelixConfig::pluginDirectory().utf8(),
                                     i18n("This is the directory where, for example, vorbisrend.so is located"));
    ++row;
    engine->m_codecsdir = HelixConfig::codecsDirectory();
    m_codec = new HelixConfigEntry( parent, engine->m_codecsdir, 
                                     this, row, 
                                     i18n("Helix/Realplay codecs directory"), 
                                     HelixConfig::codecsDirectory().utf8(),
                                     i18n("This is the directory where, for example, cvt1.so is located"));
    ++row;
    grid->addMultiCellWidget( new KSeparator( KSeparator::Horizontal, parent ), row, row, 0, 1 );

    // lets find the logo if we can
    QPixmap *pm = 0;
    QString logo = HelixConfig::coreDirectory();
    if (logo[logo.length() - 1] == '/')
       logo.append("../share/");
    else
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
    unsigned long ver;
    row = 0;
    for (int i=0; i<n; i++)
    {
       engine->getPluginInfo(i, description, copyright, moreinfourl, ver);
       le->append(QString(description));
       le->append(QString(copyright));
       le->append(QString(moreinfourl));
       le->append(QString(" "));
    }

    le->setReadOnly(true);
    le->setContentsPos(0,0);
}

bool
HelixConfigDialog::hasChanged() const
{
   for( QPtrListIterator<HelixConfigEntry> it( entries ); *it != 0; ++it )
      if ( (*it)->isChanged() )
         return true;
   if (m_core->isChanged() || m_plugin->isChanged() || m_codec->isChanged())
      return true;

   return false;
}

bool
HelixConfigDialog::isDefault() const
{
   return false;
}

void
HelixConfigDialog::save()
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

   // not really doing anything here yet
   for( HelixConfigEntry *entry = entries.first(); entry; entry = entries.next() )
   {
      if( entry->isChanged() )
      {
         debug() << "Apply: " << entry->key() << "\n";
         //if (!strcmp(entry->key(), "

         entry->setUnchanged();
      }
   }

   if (writeIt)
   {
      HelixConfig::writeConfig();
      // reinit...
      m_engine->init();
   }

}

#include "helix-configdialog.moc"

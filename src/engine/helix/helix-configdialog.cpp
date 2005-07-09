#include <qlabel.h>
#include <qlayout.h>
#include <qscrollview.h>
#include <qspinbox.h>
#include <qtooltip.h>

#include <klineedit.h>
#include <kseparator.h>
#include <klocale.h>

#include "helix-configdialog.h"
#include "helix-engine.h"

#include "config/helixconfig.h"

//#include <iostream>

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
   : m_valueChanged( false )
     , m_stringValue( defaultvalue )
{
    QGridLayout *grid = (QGridLayout*)parent->layout();
    QWidget *w = 0;

    w = new KLineEdit( m_stringValue, parent );
    connect( w, SIGNAL(textChanged( const QString& )), this, SLOT(slotStringChanged( const QString& )) );
    connect( w, SIGNAL(textChanged( const QString& )), pluginConfig, SIGNAL(viewChanged()) );
    
    QToolTip::add( w, "<qt>" + tooltip );
    
    QLabel* d = new QLabel( description + ':', parent );
    d->setAlignment( QLabel::WordBreak | QLabel::AlignVCenter );
    
    grid->addWidget( w, row, 1 );
    grid->addWidget( d, row, 0 );
}

HelixConfigEntry::HelixConfigEntry( QWidget *parent, 
                                    QString &str,
                                    amaroK::PluginConfig *pluginConfig, 
                                    int row, 
                                    const QString & description, 
                                    const char *defaultvalue,
                                    const QString & tooltip)
   : m_valueChanged( false )
     , m_stringValue( defaultvalue )
{
    QGridLayout *grid = (QGridLayout*)parent->layout();
    QWidget *w = 0;

    m_key = str;

    w = new KLineEdit( str, parent );
    connect( w, SIGNAL(textChanged( const QString& )), this, SLOT(slotStringChanged( const QString& )) );
    connect( w, SIGNAL(textChanged( const QString& )), pluginConfig, SIGNAL(viewChanged()) );
    
    QToolTip::add( w, "<qt>" + tooltip );
    
    QLabel* d = new QLabel( description + ':', parent );
    d->setAlignment( QLabel::WordBreak | QLabel::AlignVCenter );
    
    grid->addWidget( w, row, 1 );
    grid->addWidget( d, row, 0 );
}


inline void
HelixConfigEntry::slotStringChanged( const QString& )
{
    m_valueChanged = true;
}


HelixConfigDialog::HelixConfigDialog( HelixEngine *engine, QWidget *p )
   : amaroK::PluginConfig()
     , QTabWidget( p )
     , m_core(0)
     , m_plugin(0)
     , m_codecs(0)
     , m_engine( engine )
{
    int row = 0;
    QString currentPage;
    QWidget *parent = 0;
    QGridLayout *grid = 0;
    QScrollView *sv = 0;

    QString pageName( "Main" );
    pageName = pageName.left( pageName.find( '.' ) );

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
       //TODO is the viewport() not better?
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
    m_codecs = new HelixConfigEntry( parent, engine->m_codecsdir, 
                                     this, row, 
                                     i18n("Helix/Realplay codecs directory"), 
                                     HelixConfig::codecsDirectory().utf8(),
                                     i18n("This is the directory where, for example, cvt1.so is located"));
    ++row;
    grid->addMultiCellWidget( new KSeparator( KSeparator::Horizontal, parent ), row, row, 0, 1 );

    entries.setAutoDelete( true );
}

bool
HelixConfigDialog::hasChanged() const
{
   for( QPtrListIterator<HelixConfigEntry> it( entries ); *it != 0; ++it )
      if ( (*it)->isChanged() )
         return true;
   if (m_core->isChanged() || m_plugin->isChanged() || m_codecs->isChanged())
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
}

#include "helix-configdialog.moc"

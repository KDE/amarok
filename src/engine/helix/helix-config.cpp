#include <qlabel.h>
#include <qlayout.h>
#include <qscrollview.h>
#include <qspinbox.h>
#include <qtooltip.h>
#include <klineedit.h>
#include <kseparator.h>
#include "helix-config.h"
#include "helix-engine.h"
//#include <iostream>
#include "debug.h"

using namespace std;

// this code is obviously derived from xine-config :-)

HelixConfigEntry::HelixConfigEntry( QWidget *parent, 
                                    amaroK::PluginConfig *pluginConfig, 
                                    int row, 
                                    const char *description, 
                                    const char *defaultvalue,
                                    const char *tooltip)
   : m_valueChanged( false )
     , m_stringValue( defaultvalue )
     , m_str(m_stringValue)
{
    QGridLayout *grid = (QGridLayout*)parent->layout();
    QWidget *w = 0;

    w = new KLineEdit( m_stringValue, parent );
    connect( w, SIGNAL(textChanged( const QString& )), this, SLOT(slotStringChanged( const QString& )) );
    connect( w, SIGNAL(textChanged( const QString& )), pluginConfig, SIGNAL(viewChanged()) );
    
    QToolTip::add( w, "<qt>" + QString( tooltip ) );
    
    QLabel* d = new QLabel( QString::fromLocal8Bit( description ) + ':', parent );
    d->setAlignment( QLabel::WordBreak | QLabel::AlignVCenter );
    
    grid->addWidget( w, row, 1 );
    grid->addWidget( d, row, 0 );
}

HelixConfigEntry::HelixConfigEntry( QWidget *parent, 
                                    QCString &str,
                                    amaroK::PluginConfig *pluginConfig, 
                                    int row, 
                                    const char *description, 
                                    const char *defaultvalue,
                                    const char *tooltip)
   : m_valueChanged( false )
     , m_stringValue( defaultvalue )
     , m_str(str)
{
    QGridLayout *grid = (QGridLayout*)parent->layout();
    QWidget *w = 0;

    m_str = str;
    w = new KLineEdit( str, parent );
    connect( w, SIGNAL(textChanged( const QString& )), this, SLOT(slotStringChanged( const QString& )) );
    connect( w, SIGNAL(textChanged( const QString& )), pluginConfig, SIGNAL(viewChanged()) );
    
    QToolTip::add( w, "<qt>" + QString( tooltip ) );
    
    QLabel* d = new QLabel( QString::fromLocal8Bit( description ) + ':', parent );
    d->setAlignment( QLabel::WordBreak | QLabel::AlignVCenter );
    
    grid->addWidget( w, row, 1 );
    grid->addWidget( d, row, 0 );
}


inline void
HelixConfigEntry::slotStringChanged( const QString& val )
{
    m_str  = val.utf8();
    m_valueChanged = true;
}


HelixConfigDialog::HelixConfigDialog( HelixEngine *engine, QWidget *p )
   : amaroK::PluginConfig()
     , QTabWidget( p )
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


    // TODO: these 3 config items need to be stored persistently somewhere
    entries.append( new HelixConfigEntry( parent, engine->m_coredir, this, row, 
                                          "Helix/Realplay core directory", 
                                          "/usr/local/RealPlayer/common",
                                          "This is the directory where clntcore.so is located") );
    ++row;
    entries.append( new HelixConfigEntry( parent, engine->m_pluginsdir, this, row, 
                                          "Helix/Realplay plugins directory", 
                                          "/usr/local/RealPlayer/plugins",
                                          "This is the directory where, for example, vorbisrend.so is located") );
    ++row;
    entries.append( new HelixConfigEntry( parent, engine->m_codecsdir, this, row, 
                                          "Helix/Realplay codecs directory", 
                                          "/usr/local/RealPlayer/codecs",
                                          "This is the directory where, for example, cvt1.so is located") );
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

         entry->setUnchanged();
      }
   }
}

#include "helix-config.moc"

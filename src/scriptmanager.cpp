// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.


#include "scriptmanager.h"

#include <qdir.h>

#include <kdebug.h>
#include <kjsembed/jsconsolewidget.h>
#include <kjsembed/kjsembedpart.h>
#include <kurl.h>

using namespace KJSEmbed;

////////////////////////////////////////////////////////////////////////////////
// public 
////////////////////////////////////////////////////////////////////////////////

ScriptManager::Manager::Manager( QObject* object )
        : QObject( object )
{
    setName( "ScriptManager" );
    self = this;
    
    //KJSEmbed
    m_kjs = new KJSEmbedPart( this );
    m_kjs->addObject( this );
}


ScriptManager::Manager::~Manager()
{}


void
ScriptManager::Manager::showSelector() //static
{
    kdDebug() << k_funcinfo << endl;  
    
    if ( !Selector::instance ) {
        Selector::instance = new Selector( self->m_list );
        connect( Selector::instance, SIGNAL( signalRunScript( const QString& ) ),
                self,   SLOT( slotRun( const QString& ) ) );
        connect( Selector::instance, SIGNAL( signalStopScript( const QString& ) ),
                self,   SLOT( slotStop( const QString& ) ) );
        connect( Selector::instance, SIGNAL( signalConfigureScript( const QString& ) ),
                self,   SLOT( slotConfigure( const QString& ) ) );
    }
                    
    Selector::instance->show();
}


void
ScriptManager::Manager::showConsole() //static
{
    JSConsoleWidget* console = self->m_kjs->view();
    console->show();
}


void
ScriptManager::Manager::addObject( QObject* object )
{
    m_kjs->addObject( object );
}


void
ScriptManager::Manager::slotRun( const QString& path )
{
    kdDebug() << k_funcinfo << endl;  

    KURL url;
    url.setPath( path );
    QDir::setCurrent( url.directory() );
    
    kdDebug() << "Running script: " << path << endl;
    QString script = self->m_kjs->loadFile( path );
    self->m_kjs->view()->execute( script );
}


void
ScriptManager::Manager::slotStop( const QString& str )
{
    kdDebug() << k_funcinfo << endl;  

    emit stop( str );
}


void
ScriptManager::Manager::slotConfigure( const QString& path )
{
    kdDebug() << k_funcinfo << endl;  

    emit configure( path );
}


////////////////////////////////////////////////////////////////////////////////
// private 
////////////////////////////////////////////////////////////////////////////////

ScriptManager::Manager*
ScriptManager::Manager::self;

   
#include "scriptmanager.moc"



// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.

#include "config.h"
#ifdef HAVE_KJSEMBED

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

ScriptManager::Manager*
ScriptManager::Manager::s_instance = 0;


ScriptManager::Manager::Manager( QObject* object )
        : QObject( object, "ScriptManager" )
{
    s_instance = this;
    
    //KJSEmbed
    m_kjs = new KJSEmbedPart( this );
    m_kjs->addObject( this );
}


void
ScriptManager::Manager::showSelector()
{
    kdDebug() << k_funcinfo << endl;  
    
    if ( !Selector::instance ) {
        Selector::instance = new Selector( m_list );
        connect( Selector::instance, SIGNAL( signalRunScript( const QString& ) ),
                this,   SLOT( slotRun( const QString& ) ) );
        connect( Selector::instance, SIGNAL( signalStopScript( const QString& ) ),
                this,   SLOT( slotStop( const QString& ) ) );
        connect( Selector::instance, SIGNAL( signalConfigureScript( const QString& ) ),
                this,   SLOT( slotConfigure( const QString& ) ) );
    }
                    
    Selector::instance->show();
}


void
ScriptManager::Manager::showConsole()
{
    JSConsoleWidget* console = m_kjs->view();
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
    QString script = m_kjs->loadFile( path );
    m_kjs->view()->execute( script );
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

   
#include "scriptmanager.moc"

#endif /*HAVE_KJSEMBED*/


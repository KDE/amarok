// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.


#include "scriptmanager.h"

#include <qdir.h>

#include <kdebug.h>
#include <krun.h>
#include <ktextedit.h>
#include <kurl.h>


////////////////////////////////////////////////////////////////////////////////
// public
////////////////////////////////////////////////////////////////////////////////

ScriptManager::Manager*
ScriptManager::Manager::s_instance = 0;


ScriptManager::Manager::Manager( QObject* object )
        : QObject( object, "ScriptManager" )
{
    s_instance = this;
}


void
ScriptManager::Manager::addObject( QObject* object )
{
//     m_kjs->addObject( object );
}


////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void
ScriptManager::Manager::showSelector() //SLOT
{
    kdDebug() << k_funcinfo << endl;

    if ( !Selector::instance )
    {
        Selector::instance = new Selector( m_list );
        connect( Selector::instance, SIGNAL( signalEditScript( const QString& ) ),
                this,   SLOT( slotEdit( const QString& ) ) );
        connect( Selector::instance, SIGNAL( signalRunScript( const QString& ) ),
                this,   SLOT( slotRun( const QString& ) ) );
        connect( Selector::instance, SIGNAL( signalStopScript( const QString& ) ),
                this,   SLOT( slotStop( const QString& ) ) );
        connect( Selector::instance, SIGNAL( signalConfigureScript( const QString& ) ),
                this,   SLOT( slotConfigure( const QString& ) ) );
    }

    Selector::instance->show();
}


////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void
ScriptManager::Manager::slotEdit( const QString& path ) //SLOT
{
    kdDebug() << k_funcinfo << endl;

    QFile file( path );

    if ( file.open( IO_ReadWrite ) ) {
        KTextEdit* editor = new KTextEdit();
        QTextStream stream( &file );
        editor->setText( stream.read() );
        editor->setTextFormat( QTextEdit::PlainText );
        editor->resize( 640, 480 );
        editor->show();
    }
}


void
ScriptManager::Manager::slotRun( const QString& path ) //SLOT
{
    kdDebug() << k_funcinfo << endl;

    KURL url;
    url.setPath( path );
    QDir::setCurrent( url.directory() );
    kdDebug() << "Running script: " << path << endl;

    KRun* script = new KRun( url );
}


void
ScriptManager::Manager::slotStop( const QString& str ) //SLOT
{
    kdDebug() << k_funcinfo << endl;

    emit stop( str );
}


void
ScriptManager::Manager::slotConfigure( const QString& path ) //SLOT
{
    kdDebug() << k_funcinfo << endl;

    emit configure( path );
}


#include "scriptmanager.moc"


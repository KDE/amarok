/***************************************************************************
 *   Copyright (C) 2005 Max Howell <max.howell@methylblue.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "amarok.h"
#include "amarokconfig.h"
#include "crashhandler.h"

#include <kapplication.h> //invokeMailer()
#include <kdebug.h>       //kBacktrace()
#include <kdeversion.h>
#include <klocale.h>
#include <k3tempfile.h>
#include <ktoolinvocation.h>

#include <QFile>
#include <QRegExp>
#include <q3textstream.h>
#include <qglobal.h> //qVersion()
#include <Q3CString>


#include <cstdio>         //popen, fread
#include <iostream>
#include <sys/types.h>    //pid_t
#include <sys/wait.h>     //waitpid
#include <taglib.h>
#include <unistd.h>       //write, getpid



#ifndef TAGLIB_PATCH_VERSION
// seems to be wheel's style
#define TAGLIB_PATCH_VERSION 0
#endif


namespace Amarok
{
    #if 0
    class CrashHandlerWidget : public KDialog {
    public:
        CrashHandlerWidget();
    };
    #endif

    static QString
    runCommand( const Q3CString &command )
    {
        static const uint SIZE = 40960; //40 KiB
        static char stdoutBuf[ SIZE ] = {0};

//        std::cout << "Running: " << command << std::endl;

        FILE *process = ::popen( command, "r" );
        if ( process )
        {
            stdoutBuf[ std::fread( static_cast<void*>( stdoutBuf ), sizeof(char), SIZE-1, process ) ] = '\0';
            ::pclose( process );
        }
        return QString::fromLocal8Bit( stdoutBuf );
    }

    void
    Crash::crashHandler( int /*signal*/ )
    {
        // we need to fork to be able to get a
        // semi-decent bt - I dunno why
        const pid_t pid = ::fork();

        if( pid < 0 )
        {
            std::cout << "forking crash reporter failed\n";
            // continuing now can't do no good
            _exit( 1 );
        }
        else if ( pid == 0 )
        {
            // we are the child process (the result of the fork)
            std::cout << "Amarok is crashing...\n";

            QString subject = APP_VERSION " ";
            QString body = i18n(
                    "Amarok has crashed! We are terribly sorry about this :(\n\n"
                    "But, all is not lost! You could potentially help us fix the crash. "
                    "Information describing the crash is below, so just click send, "
                    "or if you have time, write a brief description of how the crash happened first.\n\n"
                    "Many thanks.\n\n" );
            body += i18n( "\n\n\n\n\n\n"
                    "The information below is to help the developers identify the problem, "
                    "please do not modify it.\n\n\n\n" );


            body += "======== DEBUG INFORMATION  =======\n"
                    "Version:    " APP_VERSION "\n"
                    "Engine:     %1\n"
                    "Build date: " __DATE__ "\n"
                    "CC version: " __VERSION__ "\n" //assuming we're using GCC
                    "KDElibs:    " KDE_VERSION_STRING "\n"
                    "Qt:         %2\n"
                    "TagLib:     %3.%4.%5\n"
                    "CPU count:  %6\n";

            QString cpucount = "unknown";
#ifdef __linux__
            QString line;
            uint cpuCount = 0;
            QFile cpuinfo( "/proc/cpuinfo" );
            if ( cpuinfo.open( QIODevice::ReadOnly ) ) {
                char cline[1024];
                while ( cpuinfo.readLine( cline, sizeof(cline) ) != -1 ) {
                    line = cline;
                    if ( line.startsWith( "processor" ) ) {
                        ++cpuCount;
                    }
                }
            }
            cpucount = QString::number( cpuCount );
#endif


            body = body.arg( AmarokConfig::soundSystem() )
                    .arg( qVersion() )
                    .arg( TAGLIB_MAJOR_VERSION )
                    .arg( TAGLIB_MINOR_VERSION )
                    .arg( TAGLIB_PATCH_VERSION )
                    .arg( cpucount );

            #ifdef NDEBUG
            body += "NDEBUG:     true";
            #endif
            body += '\n';

            /// obtain the backtrace with gdb

            K3TempFile temp;
            temp.setAutoDelete( true );

            const int handle = temp.handle();

//             QCString gdb_command_string =
//                     "file amarokapp\n"
//                     "attach " + QCString().setNum( ::getppid() ) + "\n"
//                     "bt\n" "echo \\n\n"
//                     "thread apply all bt\n";

            const Q3CString gdb_batch =
                    "bt\n"
                    "echo \\n\\n\n"
                    "bt full\n"
                    "echo \\n\\n\n"
                    "echo ==== (gdb) thread apply all bt ====\\n\n"
                    "thread apply all bt\n";

            ::write( handle, gdb_batch, gdb_batch.length() );
            ::fsync( handle );

            // so we can read stderr too
            ::dup2( fileno( stdout ), fileno( stderr ) );


            Q3CString gdb;
            gdb  = "gdb --nw -n --batch -x ";
            gdb += temp.name().toLatin1();
            gdb += " amarokapp ";
            gdb += Q3CString().setNum( ::getppid() );

            QString bt = runCommand( gdb );

            /// clean up
            bt.remove( "(no debugging symbols found)..." );
            bt.remove( "(no debugging symbols found)\n" );
            bt.replace( QRegExp("\n{2,}"), "\n" ); //clean up multiple \n characters
            bt.trimmed();

            /// analyze usefulness
            bool useful = true;
            const QString fileCommandOutput = runCommand( "file `which amarokapp`" );

            if( fileCommandOutput.indexOf( "not stripped", false ) == -1 )
                subject += "[___stripped]"; //same length as below
            else
                subject += "[NOTstripped]";

            if( !bt.isEmpty() ) {
                const int invalidFrames = bt.count( QRegExp("\n#[0-9]+\\s+0x[0-9A-Fa-f]+ in \\?\\?") );
                const int validFrames = bt.count( QRegExp("\n#[0-9]+\\s+0x[0-9A-Fa-f]+ in [^?]") );
                const int totalFrames = invalidFrames + validFrames;

                if( totalFrames > 0 ) {
                    const double validity = double(validFrames) / totalFrames;
                    subject += QString("[validity: %1]").arg( validity, 0, 'f', 2 );
                    if( validity <= 0.5 ) useful = false;
                }
                subject += QString("[frames: %1]").arg( totalFrames, 3 /*padding*/ );

                if( bt.indexOf( QRegExp(" at \\w*\\.cpp:\\d+\n") ) >= 0 )
                    subject += "[line numbers]";
            }
            else
                useful = false;

            subject += QString("[%1]").arg( AmarokConfig::soundSystem().remove( QRegExp("-?engine") ) );

            std::cout << subject.toLatin1().constData() << std::endl;


            //TODO -fomit-frame-pointer buggers up the backtrace, so detect it
            //TODO -O optimization can rearrange execution and stuff so show a warning for the developer
            //TODO pass the CXXFLAGS used with the email

            if( useful ) {
                body += "==== file `which amarokapp` =======\n";
                body += fileCommandOutput + "\n\n";
                body += "==== (gdb) bt =====================\n";
                body += bt + "\n\n";
                body += "==== kBacktrace() ================\n";
                body += kBacktrace();

                //TODO startup notification
                KToolInvocation::invokeMailer(
                        /*to*/          "amarok-backtraces@lists.sf.net",
                        /*cc*/          QString(),
                        /*bcc*/         QString(),
                        /*subject*/     subject,
                        /*body*/        body,
                        /*messageFile*/ QString(),
                        /*attachURLs*/  QStringList(),
                        /*startup_id*/  "" );
            }
            else {
                std::cout << i18n( "\nAmarok has crashed! We are terribly sorry about this :(\n\n"
                                   "But, all is not lost! Perhaps an upgrade is already available "
                                   "which fixes the problem. Please check your distribution's software repository.\n" ).toLocal8Bit().constData();
            }

            //_exit() exits immediately, otherwise this
            //function is called repeatedly ad finitum
            ::_exit( 255 );
        }

        else {
            // we are the process that crashed

            ::alarm( 0 );

            // wait for child to exit
            ::waitpid( pid, NULL, 0 );
            ::_exit( 253 );
        }
    }
}


#if 0

#include <QLabel>
#include <QLayout>
#include <q3vbox.h>
#include <kdialog.h>
#include <kpushbutton.h>
#include <kstdguiitem.h>
#include <kstandarddirs.h>
#include <ktoolinvocation.h>

Amarok::CrashHandlerWidget::CrashHandlerWidget()
{
    Q3BoxLayout *layout = new Q3HBoxLayout( this, 18, 12 );

    {
        Q3BoxLayout *lay = new Q3VBoxLayout( layout );
        QLabel *label = new QLabel( this );
        label->setPixmap( KStandardDirs::locate( "data", "drkonqi/pics/konqi.png" ) );
        label->setFrameStyle( Q3Frame::Plain | Q3Frame::Box );
        lay->add( label );
        lay->addItem( new QSpacerItem( 3, 3, QSizePolicy::Minimum, QSizePolicy::Expanding ) );
    }

    layout = new Q3VBoxLayout( layout, 6 );

    layout->add( new QLabel( /*i18n*/(
            "<p>" "Amarok has crashed! We are terribly sorry about this :("
            "<p>" "However you now have an opportunity to help us fix this crash so that it doesn't "
                  "happen again! Click <b>Send Email</b> and Amarok will prepare an email that you "
                  "can send to us that contains information about the crash, and we'll try to fix it "
                  "as soon as possible."
            "<p>" "Thanks for choosing Amarok.<br>" ), this ) );

    layout = new Q3HBoxLayout( layout, 6 );

    layout->addItem( new QSpacerItem( 6, 6, QSizePolicy::Expanding ) );
    layout->add( new KPushButton( KGuiItem( i18n("Send Email"), "mail_send" ), this, "email" ) );
    layout->add( new KPushButton( KStandardGuiItem::close(), this, "close" ) );

    child<QPushButton*>("email")->setDefault( true );

    connect( child( "email" ), SIGNAL(clicked()), SLOT(accept()) );
    connect( child( "close" ), SIGNAL(clicked()), SLOT(reject()) );

    setCaption( i18n("Crash Handler") );
    setFixedSize( sizeHint() );
}
#endif

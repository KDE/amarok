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
#include <kdebug.h>       //kdBacktrace()
#include <kdeversion.h>
#include <klocale.h>
#include <ktempfile.h>

#include <qfile.h>
#include <qregexp.h>
#include <qtextstream.h>
#include <qglobal.h> //qVersion()

#include <cstdio>         //popen, fread
#include <iostream>
#include <sys/types.h>    //pid_t
#include <sys/wait.h>     //waitpid
#include <taglib/taglib.h>
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
    runCommand( const QCString &command )
    {
        static const uint SIZE = 40960; //40 KiB
        static char stdoutBuf[ SIZE ] = {0};

        std::cout << "Running: " << command << std::endl;

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
            if ( cpuinfo.open( IO_ReadOnly ) ) {
                while ( cpuinfo.readLine( line, 20000 ) != -1 ) {
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

            KTempFile temp;
            temp.setAutoDelete( true );

            const int handle = temp.handle();

//             QCString gdb_command_string =
//                     "file amarokapp\n"
//                     "attach " + QCString().setNum( ::getppid() ) + "\n"
//                     "bt\n" "echo \\n\n"
//                     "thread apply all bt\n";

            const QCString gdb_batch =
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


            QCString gdb;
            gdb  = "gdb --nw -n --batch -x ";
            gdb += temp.name().latin1();
            gdb += " amarokapp ";
            gdb += QCString().setNum( ::getppid() );

            QString bt = runCommand( gdb );

            /// clean up
            bt.remove( "(no debugging symbols found)..." );
            bt.remove( "(no debugging symbols found)\n" );
            bt.replace( QRegExp("\n{2,}"), "\n" ); //clean up multiple \n characters
            bt.stripWhiteSpace();

            /// analyze usefulness
            bool useful = true;
            const QString fileCommandOutput = runCommand( "file `which amarokapp`" );

            if( fileCommandOutput.find( "not stripped", false ) == -1 )
                subject += "[___stripped]"; //same length as below
            else
                subject += "[NOTstripped]";

            if( !bt.isEmpty() ) {
                const int invalidFrames = bt.contains( QRegExp("\n#[0-9]+\\s+0x[0-9A-Fa-f]+ in \\?\\?") );
                const int validFrames = bt.contains( QRegExp("\n#[0-9]+\\s+0x[0-9A-Fa-f]+ in [^?]") );
                const int totalFrames = invalidFrames + validFrames;

                if( totalFrames > 0 ) {
                    const double validity = double(validFrames) / totalFrames;
                    subject += QString("[validity: %1]").arg( validity, 0, 'f', 2 );
                    if( validity <= 0.5 ) useful = false;
                }
                subject += QString("[frames: %1]").arg( totalFrames, 3 /*padding*/ );

                if( bt.find( QRegExp(" at \\w*\\.cpp:\\d+\n") ) >= 0 )
                    subject += "[line numbers]";
            }
            else
                useful = false;

            subject += QString("[%1]").arg( AmarokConfig::soundSystem().remove( QRegExp("-?engine") ) );

            std::cout << subject.latin1() << std::endl;


            //TODO -fomit-frame-pointer buggers up the backtrace, so detect it
            //TODO -O optimization can rearrange execution and stuff so show a warning for the developer
            //TODO pass the CXXFLAGS used with the email

            if( useful ) {
                body += "==== file `which amarokapp` =======\n";
                body += fileCommandOutput + "\n\n";
                body += "==== (gdb) bt =====================\n";
                body += bt + "\n\n";
                body += "==== kdBacktrace() ================\n";
                body += kdBacktrace();

                //TODO startup notification
                kapp->invokeMailer(
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
                                   "which fixes the problem. Please check your distribution's software repository.\n" ).local8Bit();
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

#include <qlabel.h>
#include <qlayout.h>
#include <qvbox.h>
#include <kdialog.h>
#include <kpushbutton.h>
#include <kstdguiitem.h>
#include <kstandarddirs.h>

Amarok::CrashHandlerWidget::CrashHandlerWidget()
{
    QBoxLayout *layout = new QHBoxLayout( this, 18, 12 );

    {
        QBoxLayout *lay = new QVBoxLayout( layout );
        QLabel *label = new QLabel( this );
        label->setPixmap( locate( "data", "drkonqi/pics/konqi.png" ) );
        label->setFrameStyle( QFrame::Plain | QFrame::Box );
        lay->add( label );
        lay->addItem( new QSpacerItem( 3, 3, QSizePolicy::Minimum, QSizePolicy::Expanding ) );
    }

    layout = new QVBoxLayout( layout, 6 );

    layout->add( new QLabel( /*i18n*/(
            "<p>" "Amarok has crashed! We are terribly sorry about this :("
            "<p>" "However you now have an opportunity to help us fix this crash so that it doesn't "
                  "happen again! Click <b>Send Email</b> and Amarok will prepare an email that you "
                  "can send to us that contains information about the crash, and we'll try to fix it "
                  "as soon as possible."
            "<p>" "Thanks for choosing Amarok.<br>" ), this ) );

    layout = new QHBoxLayout( layout, 6 );

    layout->addItem( new QSpacerItem( 6, 6, QSizePolicy::Expanding ) );
    layout->add( new KPushButton( KGuiItem( i18n("Send Email"), "mail_send" ), this, "email" ) );
    layout->add( new KPushButton( KStdGuiItem::close(), this, "close" ) );

    static_cast<QPushButton*>(child("email"))->setDefault( true );

    connect( child( "email" ), SIGNAL(clicked()), SLOT(accept()) );
    connect( child( "close" ), SIGNAL(clicked()), SLOT(reject()) );

    setCaption( i18n("Crash Handler") );
    setFixedSize( sizeHint() );
}
#endif

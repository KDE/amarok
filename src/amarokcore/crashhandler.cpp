//Copyright (C) 2005 Max Howell <max.howell@methylblue.com>
//Licensed as described in the COPYING accompanying this distribution
//

namespace std
{
    #include <stdlib.h>
    #include <unistd.h>
    #include <execinfo.h>
}

#include "amarok.h"
#include "amarokconfig.h"
#include "crashhandler.h"
#include "debug.h"
#include <kapplication.h>
#include <kdeversion.h>
#include <klocale.h>
#include <qfile.h>
#include <qtextstream.h>

namespace amaroK
{
    #if 0
    class CrashHandlerWidget : public KDialog {
    public:
        CrashHandlerWidget();
    };
    #endif

    void
    Crash::crashHandler( int /*signal*/ )
    {
        debug() << "amaroK is crashing...\n";

        {
            #define LINES 256

            QString backtrace = "Due to b0rkage, the crash-function gets shown like: [0xffffe420]\n\n";
            QString path = amaroK::saveLocation() + "backtrace";
            QString body = i18n(
                    "amaroK has crashed! We're terribly sorry about this :(\n\n"
                    "But, all is not lost! You could potentially help us fix the crash. "
                    "amaroK has attached a backtrace that describes the crash, so just click send, "
                    "or if you have time, write a brief description of how the crash happened first.\n\n"
                    "Many thanks.\n\n" );

            body += "Engine       %1\n"
                    "Build date:  " __DATE__ "\n"
                    "CC version: " __VERSION__ "\n" //assuming we're using GCC
                    "KDElibs:     " KDE_VERSION_STRING "\n";

            {   /// obtain backtrace
                void *array[ LINES ];
                size_t size = std::backtrace( array, LINES );

                if ( size ) {
                    char **strings = std::backtrace_symbols( array, size );

                    for( size_t i = 0; i < size; i++ ) {
                        backtrace += QString::number( i ) + ": ";
                        backtrace += QString::fromLatin1( strings[i] );
                        backtrace += '\n'; }

                    std::free( strings );
                }
                else
                    backtrace = "No backtrace available :(";
            }

            {   /// write a file to contain the backtrace
                QFile file( path );
                file.open( IO_WriteOnly );
                QTextStream( &file ) << backtrace;
            }

            //TODO startup notification
            kapp->invokeMailer(
                    /*to*/          "amarok-backtraces@lists.sf.net",
                    /*cc*/          QString(),
                    /*bcc*/         QString(),
                    /*subject*/     "amaroK " APP_VERSION " Crash Backtrace",
                    /*body*/        body.arg( AmarokConfig::soundSystem() ),
                    /*messageFile*/ QString(),
                    /*attachURLs*/  QStringList( path ),
                    /*startup_id*/  "" );
        }

        //_exit() exits immediately, otherwise this
        //function is called repeatedly ad finitum
        std::_exit( 255 );
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

amaroK::CrashHandlerWidget::CrashHandlerWidget()
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
            "<p>" "amaroK has crashed! We're terribly sorry about this :("
            "<p>" "However you now have an opportunity to help us fix this crash so that it doesn't "
                  "happen again! Click <b>Send Email</b> and amaroK will prepare an email that you "
                  "can send to us that contains information about the crash, and we'll try to fix it "
                  "as soon as possible."
            "<p>" "Thanks for choosing amaroK.<br>" ), this ) );

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

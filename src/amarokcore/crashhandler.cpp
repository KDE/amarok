//Copyright (C) 2005 Max Howell <max.howell@methylblue.com>
//Licensed as described in the COPYING accompanying this distribution
//

#include "amarok.h"
#include "amarokconfig.h"
#include "crashhandler.h"
#include "debug.h"
#include <kapplication.h>
#include <kdialog.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <kstdguiitem.h>
#include <kstandarddirs.h>
#include <qfile.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qtextstream.h>
#include <qvbox.h>

namespace std
{
    #include <stdlib.h>
    #include <unistd.h>
    #include <execinfo.h>
}

namespace amaroK
{
    class CrashHandlerWidget : public KDialog {
    public:
        CrashHandlerWidget();
    };

    void
    Crash::crashHandler( int /*signal*/ )
    {
        DEBUG_FUNC_INFO

        switch( CrashHandlerWidget().exec() )
        {
        case QDialog::Accepted: {
            #define LINES 40

            QString backtrace;
            QString path = amaroK::saveLocation() + "backtrace";
            QString body =
                    "Please enter a BRIEF DESCRIPTION describing the crash's cause. Thanks!\n\n"
                    "Engine       %1\n"
                    "Build date:  " __DATE__ "\n"
                    "GCC version: " __VERSION__; //assuming we're using GCC

            {   /// obtain backtrace
                void *array[ LINES ];
                size_t size;
                char **strings;
                size_t i;

                size = std::backtrace( array, LINES );
                strings = std::backtrace_symbols( array, size );

                for (i = 0; i < size; i++) {
                    backtrace += QString::fromLatin1( strings[i] );
                    backtrace += '\n'; }

                std::free( strings );
            }

            {   /// write a file to contain the backtrace
                QFile file( path );
                file.open( IO_WriteOnly );
                QTextStream( &file ) << backtrace;
            }

            //TODO startup notification
            kapp->invokeMailer(
                    /*to*/          "amarok-devel@lists.sf.net",
                    /*cc*/          QString(),
                    /*bcc*/         QString(),
                    /*subject*/     "amaroK " APP_VERSION " Crash Backtrace",
                    /*body*/        body.arg( AmarokConfig::soundSystem() ),
                    /*messageFile*/ QString(),
                    /*attachURLs*/  QStringList( path ),
                    /*startup_id*/  "" );
            }
        }

        //_exit() exits immediately, otherwise this
        //function is called repeatedly ad finitum
        std::_exit( 255 );
    }
}

amaroK::CrashHandlerWidget::CrashHandlerWidget()
{
    DEBUG_FUNC_INFO

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

    layout->add( new QLabel( i18n(
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

/***************************************************************************
 *   Copyright (C) 2005 by Max Howell <max.howell@methylblue.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef STATUSBARBASE_H
#define STATUSBARBASE_H

#include "amarok_export.h"
#include "progressBar.h" //convenience

#include <KStatusBar>

#include <QEvent>
#include <QMap>        //stack allocated

class StatusBarMessageLabel;
class QLabel;
class QProgressBar;
class KSqueezedTextLabel;
class KJob;

namespace KDE
{
    class OverlayWidget;
    typedef QMap<const QObject*, ProgressBar*> ProgressMap;

    /**
     * @class KDE::StatusBar
     * @short advanced statusBar
     * @author Max Howell <max.howell@methylblue.com>
     *
     * Like a normal KStatusBar, but add widgets directly:
     *
     *    new QLabel( text, statusbar );
     *
     * The StatusBar.has some handy progress monitoring behaviour, use like so:
     *
     *    statusbar->newProgressOperation( myObject )
     *          .setDescription( i18n("MyProgressOperation") )
     *          .setStatus( i18n("Stage1") )
     *          .setAbortSlot( myObject, SLOT(abort()) )
     *          .setTotalSteps( 100 );
     *
     * The newProgressOperation function returns a KDE::ProgressBar, which is
     * a QProgressBar with some additional functions that return ProgressBar&,
     * so you can chain the mutators like above. @see KDE::ProgressBar
     *
     * After this point you can use setProgress( QObject*, int steps ) to update
     * the progress for this progress operation. Only one progress operation per
     * QObject!
     *
     * You can also follow KIO::Jobs, with built in error handling, and
     * ThreadManager::Jobs have built in thread-safe progress handling.
     *
     * You can show long status/error messages using longMessage(), these are
     * meant to be instead of showing an irritating, interuptory KMessageBox.
     *
     * Caveats:
     * This only looks sensible if the statusbar is at the bottom of the screen
     *
     * @see KDE::ProgressBar
     */

    class StatusBar : public KStatusBar
    {
        Q_OBJECT

    public:
        explicit StatusBar( QWidget *parent, const char *name = "mainStatusBar" );

        enum MessageType { Information, Question, Sorry, Warning, Error, ShowAgainCheckBox, None, OperationCompleted };

        /**
         * Start a progress operation, if owner is 0, the return value is
         * undefined - the application will probably crash.
         * @param owner controls progress for this operation
         * @return the progressBar so you can configure its parameters
         * @see setProgress( QObject*, int )
         * @see incrementProgress( QObject* )
         * @see setProgressStatus( const QObject*, const QString& )
         */
        AMAROK_EXPORT ProgressBar &newProgressOperation( QObject *owner );

        /**
         * Monitor progress for a KIO::Job, very handy.
         */
        AMAROK_EXPORT ProgressBar &newProgressOperation( KJob* );

        AMAROK_EXPORT void incrementProgressTotalSteps( const QObject *owner, int inc = 1 );
        AMAROK_EXPORT void incrementProgress( const QObject *owner );
        AMAROK_EXPORT void setProgressStatus( const QObject *owner, const QString &text );

    public slots:
        /**
         * The StatusBar.has a region where you can display a mainMessage.
         * It persists after all other message-types are displayed
         */
        void setMainText( const QString &text );


         /**
         * Set a small, optional, 16x16 pixel icon that can be
         * shown alongside the mainText
         */
        void setMainTextIcon( QPixmap icon );

        void hideMainTextIcon();

        /// resets mainText if you've done a shortMessage
        void resetMainText();

        /**
         * Shows a non-invasive messgeBox style message that the user has to dismiss
         * but it doesn't interrupt whatever he is doing at the current time.
         * Generally you should use these, as it is very easy for a user to not notice
         * statusBar messages.
         */
        AMAROK_EXPORT void longMessage( const QString &text, MessageType type = Information );

        void longMessage( const QString &text, MessageType type = Information );


        /**
         * Shows a short message, with a button that can be pushed to show a long
         * message
         */
        AMAROK_EXPORT void shortLongMessage( const QString &_short, const QString &_long, MessageType type = Information );

        /**
         * Set a temporary message over the mainText label, for 5 seconds.
         * ONLY USE FOR STATUS MESSAGES! ie "Buffering...", "Connecting to source..."
         */
        AMAROK_EXPORT void shortMessage( const QString &text, bool longShort = false );

        void shortMessageThreadSafe( const QString &text );

        /** Stop anticipating progress from sender() */
        void endProgressOperation();

        /** Stop anticipating progress from @param owner */
        AMAROK_EXPORT void endProgressOperation( QObject *owner );

        /**
         * Convenience function works like setProgress( QObject*, int )
         * Uses the return value from sender() to determine the owner of
         * the progress bar in question
         */
        void setProgress( int steps );
        void setProgress( const QObject *owner, int steps );
        /**
         * Convenience function works like setTotalSteps( QObject*, int )
         * Uses the return value from sender() to determine the owner of
         * the progress bar in question
         */
        //void setTotalSteps( int totalSteps );

        /**
         * Convenience function works like incrementProgress( QObject* )
         * Uses the return value from sender() to determine the owner of
         * the progress bar in question
         */
        void incrementProgress();

    public slots:
        void toggleProgressWindow( bool show );
        void abortAllProgressOperations();

    private slots:
        /** For internal use against KIO::Jobs */
        void setProgress( KJob*, unsigned long percent );
        void showMainProgressBar();
        void hideMainProgressBar();
        void updateProgressAppearance();
        void showShortLongDetails();
        void popupDeleted();

    protected:
        virtual ~StatusBar();

        virtual void polish();
        virtual void customEvent( QEvent* );
        virtual bool event( QEvent* );

        KSqueezedTextLabel *m_mainTextLabel;
        QLabel *m_iconLabel;

    private:
        struct Message
        {
            Message() {}
            Message( const QString &_text, const MessageType _type ) : text( _text ), type( _type ), offset( 0 ) {}

            QString text;
            MessageType type;

            int offset;
        };

        void updateTotalProgress();
        ProgressBar &newProgressOperationInternal( QObject *owner );
        bool allDone(); ///@return true if all progress operations are complete
        void pruneProgressBars(); /// deletes old progress bars
        void writeLogFile( const QString &text );

        int  m_logCounter;

        QWidget *cancelButton()               { return findChild<QWidget*>( "cancelButton" ); }
        //QWidget *toggleProgressWindowButton() { return findChild<QWidget*>( "showAllProgressDetails" ); }
        QWidget *progressBox()                { return findChild<QWidget*>( "progressBox" ); }
        QWidget *shortLongButton()            { return findChild<QWidget*>( "shortLongButton" ); }

        OverlayWidget *m_popupProgress;
        QProgressBar  *m_mainProgressBar;

        ProgressMap          m_progressMap;
        QList<QWidget*>      m_messageQueue;
        QString              m_mainText;
        QString              m_shortLongText;
        MessageType          m_shortLongType;

        StatusBarMessageLabel* m_messageLabel;

        QLayout *m_otherWidgetLayout;

        QList<QString> m_longMessageQueue;
        bool popupShown;
    };
}

class ShortMessageEvent : public QEvent
{
    public:
        ShortMessageEvent( const QString& text ) : QEvent( QEvent::User ), m_text( text ) {};
        QString text() const { return m_text; }

    private:
        QString m_text;
};

class LongMessageEvent : public QEvent
{
    public:
        LongMessageEvent( const QString& text ) : QEvent( QEvent::User ), m_text( text ) {};
        QString text() const { return m_text; }

    private:
        QString m_text;
};

#endif


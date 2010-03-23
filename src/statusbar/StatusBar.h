/****************************************************************************************
 * Copyright (c) 2005 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_STATUS_BAR_H
#define AMAROK_STATUS_BAR_H

#include "EngineObserver.h"
#include "MainWindow.h"
#include "amarok_export.h"
#include "core/meta/Meta.h"
#include "statusbar/CompoundProgressBar.h"

#include <KJob>
#include <KStatusBar>
#include <KSqueezedTextLabel>

#include <QTimer>

#define SHORT_MESSAGE_DURATION 5000
#define POPUP_MESSAGE_DURATION 5000

class StatusBar;

namespace The
{
    AMAROK_EXPORT StatusBar* statusBar();
}

/**
A new, much simpler status bar as the old one really did not survive the porting from Qt3 to Qt4 very well

	@author
*/
class AMAROK_EXPORT StatusBar : public KStatusBar, public EngineObserver, public Meta::Observer
{
    Q_OBJECT
    //friend StatusBar* The::statusBar();

    static StatusBar* s_instance;

public:
    enum MessageType { Information, Question, Sorry, Warning, Error, ShowAgainCheckBox, None, OperationCompleted };

    StatusBar( QWidget * parent );
    ~StatusBar();

    static StatusBar* instance() { return s_instance; }

    void shortMessage( const QString &text );
    void longMessage( const QString &text, MessageType type = Information );

    using Observer::metadataChanged;
    void metadataChanged( Meta::TrackPtr track );

signals:
    void signalLongMessage( const QString & text, MessageType type );

public slots:
    /**
     * Start a progress operation, if owner is 0, the return value is
     * undefined - the application will probably crash.
     * @param owner controls progress for this operation
     * @return the progressBar so you can configure its parameters
     * @see setProgress( QObject*, int )
     * @see incrementProgress( QObject* )
     * @see setProgressStatus( const QObject*, const QString& )
     */
    ProgressBar *newProgressOperation( QObject *owner, const QString & description );

    /**
     * Monitor progress for a KIO::Job, very handy.
     */
    ProgressBar *newProgressOperation( KJob* job, const QString & description );


    //this stuff we just forward to the compound progress bar:

    void incrementProgressTotalSteps( const QObject *owner, int inc = 1 )
    {
        m_progressBar->incrementProgressTotalSteps( owner, inc );
    }
    void incrementProgress( const QObject *owner )
    {
        m_progressBar->incrementProgress( owner );
    }

    /**
     * Convenience function works like incrementProgress( QObject* )
     * Uses the return value from sender() to determine the owner of
     * the progress bar in question
     */
    void incrementProgress()
    {
        m_progressBar->incrementProgress( sender() );
    }

    void setProgressStatus( const QObject *owner, const QString &text )
    {
        m_progressBar->setProgressStatus( owner, text );
    }

    void setProgress( int steps )
    {
        m_progressBar->setProgress( sender(), steps );
    }

    void setProgress( const QObject *owner, int steps )
    {
        m_progressBar->setProgress( owner, steps );
    }

    /**
     * Convenience function works like setTotalSteps( QObject*, int )
     * Uses the return value from sender() to determine the owner of
     * the progress bar in question
     */

    void endProgressOperation( const QObject * owner )
    {
        m_progressBar->endProgressOperation( owner );
    }

protected:
    virtual void engineStateChanged( Phonon::State state, Phonon::State oldState = Phonon::StoppedState );
    virtual void engineNewTrackPlaying();

protected slots:
    void hideProgress();
    void nextShortMessage();
    void hideLongMessage();

private:
    void updateInfo( Meta::TrackPtr track );

    CompoundProgressBar * m_progressBar;

    KHBox * m_nowPlayingWidget;
    KSqueezedTextLabel * m_nowPlayingLabel;
    QLabel * m_nowPlayingEmblem;
    QFrame *m_separator;

    QLabel * m_playlistLengthLabel;

    QList<QString> m_shortMessageQue;

    bool m_busy;
    QTimer * m_shortMessageTimer;

    Meta::TrackPtr m_currentTrack;

private slots:
    void slotLongMessage( const QString &text, MessageType type = Information );
    void updateTotalPlaylistLength();
};

Q_DECLARE_METATYPE( StatusBar::MessageType )

#endif

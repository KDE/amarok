/****************************************************************************************
 * Copyright (c) 2004 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2009 Artur Szymiec <artur.szymiec@gmail.com>                           *
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

#ifndef AMAROK_ACTIONCLASSES_H
#define AMAROK_ACTIONCLASSES_H

#include <KAction>
#include <KMenu>
#include <KToggleAction>
#include <KSelectAction>

#include <QWeakPointer>

#include <Phonon/Global>

class KActionCollection;
class KHelpMenu;


namespace Amarok
{
    class Menu : public KMenu
    {
        Q_OBJECT
        public:
            Menu( QWidget* parent );
            static Menu *instance();
            static KMenu *helpMenu( QWidget *parent = 0 );

        private:
            static Menu       *s_instance;
            static KHelpMenu  *s_helpMenu;
    };

    class MenuAction : public KAction
    {
        public:
            MenuAction( KActionCollection*, QObject* );
    };

    class PlayPauseAction : public KToggleAction
    {
        Q_OBJECT

        public:
            PlayPauseAction( KActionCollection*, QObject* );

        private Q_SLOTS:
            void stopped();
            void paused();
            void playing();
    };

    class ToggleAction : public KToggleAction
    {
        public:
            ToggleAction( const QString &text, void ( *f ) ( bool ), KActionCollection* const ac, const char *name, QObject *parent );
            virtual void setChecked( bool b );
            virtual void setEnabled( bool b );

        private:
            void ( *m_function ) ( bool );
    };

    class SelectAction : public KSelectAction
    {
        Q_OBJECT

        public:
            SelectAction( const QString &text, void ( *f ) ( int ), KActionCollection* const ac, const char *name, QObject *parent );

            virtual void setCurrentItem( int n );
            virtual void setEnabled( bool b );
            virtual void setIcons( QStringList icons );
            virtual QString currentText() const;
            QStringList icons() const;
            QString currentIcon() const;

        protected Q_SLOTS:
            virtual void actionTriggered( QAction *a );

        private:
            void ( *m_function ) ( int );
            QStringList m_icons;
    };

    class RandomAction : public SelectAction
    {
        public:
            RandomAction( KActionCollection *ac, QObject* );
            virtual void setCurrentItem( int n );
    };

    class FavorAction : public SelectAction
    {
        public:
            FavorAction( KActionCollection *ac, QObject* );
    };

    class RepeatAction : public SelectAction
    {
        public:
            RepeatAction( KActionCollection *ac, QObject* );
    };

    class ReplayGainModeAction : public SelectAction
    {
        public:
            ReplayGainModeAction( KActionCollection *ac, QObject* );
    };

    class BurnMenu : public KMenu
    {
        Q_OBJECT

        public:
            BurnMenu( QWidget* parent );
            static KMenu *instance();

        private Q_SLOTS:
            void slotBurnCurrentPlaylist();
            void slotBurnSelectedTracks();

        private:
            static BurnMenu* s_instance;
    };


    class BurnMenuAction : public KAction
    {
        public:
            BurnMenuAction( KActionCollection*, QObject* );
            virtual QWidget* createWidget( QWidget* );
    };

    class StopAction : public KAction
    {
        Q_OBJECT
        public:
            StopAction( KActionCollection*, QObject* );

        private Q_SLOTS:
            void stopped();
            void playing();
            void stop();
    };

    class StopPlayingAfterCurrentTrackAction : public KAction
    {
        Q_OBJECT
        public:
            StopPlayingAfterCurrentTrackAction( KActionCollection*, QObject* );

        private Q_SLOTS:
            void stopPlayingAfterCurrentTrack();
    };
} /* namespace Amarok */


#endif /* AMAROK_ACTIONCLASSES_H */


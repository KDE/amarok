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

#include <QAction>
#include <QMenu>

#include <KSelectAction>
#include <KToggleAction>

#include <phonon/Global>

class KActionCollection;
class KHelpMenu;


namespace Amarok
{
    class Menu : public QMenu
    {
        Q_OBJECT
        public:
            explicit Menu( QWidget* parent );
            static Menu *instance();
            static QMenu *helpMenu( QWidget *parent = nullptr );

        private:
            static Menu       *s_instance;
            static KHelpMenu  *s_helpMenu;
    };

    class MenuAction : public QAction
    {
        public:
            MenuAction( KActionCollection*, QObject* );

            /**
             * Indicate whether the user may configure the action's shortcut.
             */
            void setShortcutConfigurable(bool configurable);
    };

    class PlayPauseAction : public KToggleAction
    {
        Q_OBJECT

        public:
            explicit PlayPauseAction( KActionCollection*, QObject* );

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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            void actionTriggered( QAction *a ) override;
#else
            void slotActionTriggered( QAction *a ) override;
#endif

        private:
            void ( *m_function ) ( int );
            QStringList m_icons;
    };

    class RandomAction : public SelectAction
    {
        public:
            RandomAction( KActionCollection *ac, QObject* );
            void setCurrentItem( int n ) override;
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

    class BurnMenu : public QMenu
    {
        Q_OBJECT

        public:
            explicit BurnMenu( QWidget* parent );
            static QMenu *instance();

        private Q_SLOTS:
            void slotBurnCurrentPlaylist();
            void slotBurnSelectedTracks();

        private:
            static BurnMenu* s_instance;
    };


    class BurnMenuAction : public QAction
    {
        public:
            BurnMenuAction( KActionCollection*, QObject* );
            virtual QWidget* createWidget( QWidget* );
    };

    class StopAction : public QAction
    {
        Q_OBJECT
        public:
            StopAction( KActionCollection*, QObject* );

        private Q_SLOTS:
            void stopped();
            void playing();
            void stop();
    };

    class StopPlayingAfterCurrentTrackAction : public QAction
    {
        Q_OBJECT
        public:
            StopPlayingAfterCurrentTrackAction( KActionCollection*, QObject* );

        private Q_SLOTS:
            void stopPlayingAfterCurrentTrack();
    };
} /* namespace Amarok */


#endif /* AMAROK_ACTIONCLASSES_H */


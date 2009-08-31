/****************************************************************************************
 * Copyright (c) 2003 Stanislav Karchebny <berkus@users.sf.net>                         *
 * Copyright (c) 2009 Kevin Funk <krf@electrostorm.net>                                 *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_SYSTRAY_H
#define AMAROK_SYSTRAY_H

#include "EngineObserver.h" //baseclass
#include "meta/Meta.h"
#include "SmartPointerList.h"

#include <QPointer>

#include <KAction>
#include <KSystemTrayIcon> //baseclass

class QEvent;
class App;


namespace Amarok {

class TrayIcon : public KSystemTrayIcon, public EngineObserver
{
    Q_OBJECT

public:
    TrayIcon( QWidget *widget );
    friend class ::App;
    
    void setVisible( bool visible );

protected:
    // reimplemented from engineobserver
    virtual void engineStateChanged( Phonon::State state, Phonon::State oldState = Phonon::StoppedState );
    virtual void engineNewTrackPlaying();
    virtual void engineNewMetaData( const QHash<qint64, QString> &newMetaData, bool trackChanged );
    virtual void engineTrackPositionChanged( long position, bool /*userSeek*/ );
    virtual void engineVolumeChanged( int percent );
    virtual void engineMuteStateChanged( bool mute );

    // get notified of 'highlight' color change
    virtual void paletteChange( const QPalette & oldPalette );

private slots:
    void slotActivated( QSystemTrayIcon::ActivationReason reason );

private:
    virtual bool event( QEvent *e );
    void setupMenu();
    void setupToolTip();

    void paintIcon( long trackPosition = -1 );
    void blendOverlay( const QPixmap &overlay );

    Meta::TrackPtr m_track;
    long m_trackLength;

    QPixmap m_baseIcon, m_grayedIcon, m_icon;
    QPixmap m_playOverlay, m_pauseOverlay;
    SmartPointerList<QAction> m_extraActions;
    QPointer<QAction> m_separator;
};

}

#endif // AMAROK_SYSTRAY_H

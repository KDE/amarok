//
// AmarokSystray
//
// Author: Stanislav Karchebny <berkus@users.sf.net>, (C) 2003
//
// Copyright: like rest of Amarok
//

#ifndef AMAROKSYSTRAY_H
#define AMAROKSYSTRAY_H

#include "engineobserver.h" //baseclass

#include <KAction>
#include <ksystemtrayicon.h>

#include <QPixmap>

class QEvent;

class App;

namespace Amarok {

class TrayIcon : public KSystemTrayIcon, public EngineObserver
{
public:
    TrayIcon( QWidget* );
    friend class ::App;

protected:
    // reimpl from engineobserver
    virtual void engineStateChanged( Engine::State state, Engine::State oldState = Engine::Empty );
    virtual void engineNewMetaData( const QHash<qint64, QString> &newMetaData, bool trackChanged );
    virtual void engineTrackPositionChanged( long position, bool /*userSeek*/ );
    // get notified of 'highlight' color change
    virtual void paletteChange( const QPalette & oldPalette );

private:
    virtual bool event( QEvent *e );
    //void setLastFm( bool );
    void setupMenu();

    // repaints trayIcon showing progress (and overlay if present)
    void paintIcon( int mergePixels = -1, bool force = false );
    // blend an overlay icon over 'sourcePixmap' and repaint trayIcon
    void blendOverlay( QPixmap &sourcePixmap );

    long trackLength, mergeLevel;
    QIcon baseIcon;
    QPixmap grayedIcon, alternateIcon;
    QPixmap playOverlay, pauseOverlay;
    QPixmap *overlay;   // the current overlay (may be NULL)
    int blinkTimerID;   // timer ID returned by QObject::startTimer()
    bool overlayVisible;// used for blinking / hiding overlay
    QList<QAction*> m_extraActions;
};

}

#endif

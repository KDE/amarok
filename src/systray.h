//
// AmarokSystray
//
// Author: Stanislav Karchebny <berkus@users.sf.net>, (C) 2003
//
// Copyright: like rest of amaroK
//

#ifndef AMAROKSYSTRAY_H
#define AMAROKSYSTRAY_H

#include <ksystemtray.h>
#include "engineobserver.h" //baseclass

namespace amaroK {

class TrayIcon : public KSystemTray, public EngineObserver
{
public:
    TrayIcon( QWidget* );
    ~TrayIcon( );

protected:
    // reimpl from engineobserver
    virtual void engineStateChanged( Engine::State state );
    virtual void engineNewMetaData( const MetaBundle &bundle, bool trackChanged );
    virtual void engineTrackPositionChanged( long position );
    // get notified of 'highlight' color change
    virtual void paletteChange( const QPalette & oldPalette );

private:
    bool event( QEvent* );
    
    enum TrayOverlay { OV_none, OV_play, OV_pause, OV_stop };

    // repaints trayIcon showing progress (and overlay if present)
    void paintIcon( int percent, TrayOverlay overlay );
    
    // blend an overlay icon over 'sourcePixmap' and repaint trayIcon
    void blendOverlay( QPixmap * sourcePixmap, TrayOverlay overlay );

    // load overlay pixmap (taken from icons/b_iconName.png)
    QPixmap * loadOverlay( const char * iconName );

    QPixmap *baseIcon, *grayedIcon, *alternateIcon;
    QPixmap *playOverlay, *pauseOverlay, *stopOverlay;
    long trackLength, trackPercent, drawnPercent;
};

}

#endif

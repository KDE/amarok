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
    void paintProgressIcon( int percent = 100 );
    QPixmap *baseIcon, *grayedIcon, *alternateIcon;
    long trackLength, trackPercent, drawnPercent;
};

}

#endif

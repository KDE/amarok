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

class KActionCollection;
class QEvent;

namespace amaroK {

class TrayIcon : public KSystemTray
{
Q_OBJECT

public:
    TrayIcon( QWidget *, KActionCollection * );

private:
    bool event( QEvent* );
};

}

#endif

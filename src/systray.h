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

namespace amaroK {

class TrayIcon : public KSystemTray
{
public:
    TrayIcon( QWidget* );

private:
    bool event( QEvent* );
};

}

#endif

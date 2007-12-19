/***************************************************************************
 * copyright            : (C) 2007 Shane King <kde@dontletsstart.com>      *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "RadioAdapter.h"
#include "core/Radio.h"
#include "LastFmService.h"


RadioAdapter::RadioAdapter( QObject *parent, const QString &username, const QString &password )
    : QObject( parent )
{
    m_radio = new Radio( this );
}


RadioAdapter::~RadioAdapter()
{
}


namespace The
{
    Radio &radio()
    {
        return *lastFmService()->radio()->m_radio;
    }
}

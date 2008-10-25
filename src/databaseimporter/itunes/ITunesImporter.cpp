/*******************************************************************************
* copyright              : (C) 2008 Seb Ruiz <ruiz@kde.org>                    *
********************************************************************************/

/*******************************************************************************
*                                                                              *
*   This program is free software; you can redistribute it and/or modify       *
*   it under the terms of the GNU General Public License as published by       *
*   the Free Software Foundation; either version 2 of the License, or          *
*   (at your option) any later version.                                        *
*                                                                              *
********************************************************************************/

#include "ITunesImporter.h"
#include "Debug.h"

#include <QLabel>

ITunesImporterConfig::ITunesImporterConfig( QWidget *parent )
    : DatabaseImporterConfig( parent )
{
    new QLabel( i18n("iTunes Library"), this );
}

ITunesImporter::ITunesImporter()
    : DatabaseImporter()
{
}

ITunesImporter::~ITunesImporter()
{
}

void
ITunesImporter::import()
{
    DEBUG_BLOCK
}

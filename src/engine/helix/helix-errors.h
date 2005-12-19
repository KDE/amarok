/***************************************************************************
 *   Copyright (C) 2005 Paul Cifarelli                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef _HELIX_ERRORS_INCLUDED
#define _HELIX_ERRORS_INCLUDED

class QString;
class HelixErrorsBase;

class HelixErrors
{
public:
   static QString *errorText(unsigned long code);

private:
   HelixErrors();
   static HelixErrorsBase *m_base;
};


#endif

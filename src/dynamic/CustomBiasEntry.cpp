/*****************************************************************************
* copyright            : (C) 2009 Leo Franchi <lfranchi@kde.org>             *
******************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "CustomBiasEntry.h"


// CLASS CustomBiasEntry
Dynamic::CustomBiasEntry::CustomBiasEntry( double wieght )
    : m_weight( wieght )
{

}

void
Dynamic::CustomBiasEntry::setWeight(int weight)
{
    m_weight = (double)weight / 100;
}

double
Dynamic::CustomBiasEntry::weight()
{
    return m_weight;
}


/***************************************************************************
    copyright            : (C) 2002, 2003, 2006 by Jochen Issing
    email                : jochen.issing@isign-softart.de
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it  under the terms of the GNU Lesser General Public License version  *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA  02110-1301  USA                                                   *
 ***************************************************************************/

#include "mp4fourcc.h"

using namespace TagLib;

MP4::Fourcc::Fourcc()
{
  m_fourcc = 0U;
}

MP4::Fourcc::Fourcc( TagLib::String fourcc )
{
  m_fourcc = 0U;

  if( fourcc.size() >= 4 )
    m_fourcc = static_cast<unsigned char>(fourcc[0]) << 24 |
               static_cast<unsigned char>(fourcc[1]) << 16 |
               static_cast<unsigned char>(fourcc[2]) <<  8 |
               static_cast<unsigned char>(fourcc[3]);
}

MP4::Fourcc::~Fourcc()
{}

TagLib::String MP4::Fourcc::toString() const
{
  TagLib::String fourcc;
  fourcc.append(static_cast<char>(m_fourcc >> 24 & 0xFF));
  fourcc.append(static_cast<char>(m_fourcc >> 16 & 0xFF));
  fourcc.append(static_cast<char>(m_fourcc >>  8 & 0xFF));
  fourcc.append(static_cast<char>(m_fourcc       & 0xFF));

  return fourcc;
}

MP4::Fourcc::operator unsigned int() const
{
  return m_fourcc;
}

bool MP4::Fourcc::operator == (unsigned int fourccB ) const
{
  return (m_fourcc==fourccB);
}

bool MP4::Fourcc::operator != (unsigned int fourccB ) const
{
  return (m_fourcc!=fourccB);
}

MP4::Fourcc& MP4::Fourcc::operator = (unsigned int fourcc )
{
  m_fourcc = fourcc;
  return *this;
}

MP4::Fourcc& MP4::Fourcc::operator = (char fourcc[4])
{
  m_fourcc = static_cast<unsigned char>(fourcc[0]) << 24 |
             static_cast<unsigned char>(fourcc[1]) << 16 |
             static_cast<unsigned char>(fourcc[2]) <<  8 |
             static_cast<unsigned char>(fourcc[3]);
  return *this;
}

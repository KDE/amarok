/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#include "jamendoservice.h"

#include "jamendocontentmodel.h"
#include "jamendoxmlparser.h"

JamendoService::JamendoService(const QString & name)
 : ServiceBase( name )
{



}


JamendoService::~JamendoService()
{
}

void JamendoService::polish()
{

    JamendoXmlParser * parser = new JamendoXmlParser("hello");
    parser->doJob();

    JamendoContentModel * model = new JamendoContentModel();
    setModel( model );

}





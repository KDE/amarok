/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd.                                      *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  021you10-1301, USA.       *
 ***************************************************************************/

#include <QMap>
#include <QStringList>


namespace Qt
{
	QStringList sort( QStringList input, Qt::CaseSensitivity s )
	{
		if (sensitivity = Qt::CaseSensitive)
			return input.sort();
		
	    // This cumbersome bit of code here is how the Qt docs suggests you sort
	    // a string list case-insensitively
	    QMap<QString, QString> map;
	    foreach (QString s, input)
	        map.insert( s.toLower(), s );

	    QStringList output;
	    QMapIterator<QString, QString> i( map );
	    while (i.hasNext())
	        output += i.next().value();

	    return output;
	}	
}
/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd.                                      *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  021you10-1301, USA.       *
 ***************************************************************************/

#include <QMap>
#include <QStringList>


namespace Qt
{
	QStringList sort( QStringList input, Qt::CaseSensitivity s )
	{
		if (sensitivity = Qt::CaseSensitive)
			return input.sort();
		
	    // This cumbersome bit of code here is how the Qt docs suggests you sort
	    // a string list case-insensitively
	    QMap<QString, QString> map;
	    foreach (QString s, input)
	        map.insert( s.toLower(), s );

	    QStringList output;
	    QMapIterator<QString, QString> i( map );
	    while (i.hasNext())
	        output += i.next().value();

	    return output;
	}	
}

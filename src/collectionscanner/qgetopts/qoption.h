#ifndef QOPTION_H
#define QOPTION_H
/***************************************************************************
 *   Copyright (C) 2008 by Vitali Lovich   *
 *   vlovich@gmail.com   *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <QString>
#include <QStringList>

/**
	@author Vitali Lovich <vlovich@gmail.com>
*/
class QOption
{
public:
	QOption(const QString & category);
	QOption(const QOption& other);
	~QOption();

	QOption& setLongCode(const QString& codeName);
	QOption& setShortCode(const QChar& code);
	QOption& setDescription(const QString & description);
	QOption& setArguments(const int& numArgs, const QChar& separator = ',', const bool& optional = false, const QStringList& argumentNames = QStringList());

	QString argumentsDescription() const;
	QString getLongCode() const;
	QString getShortCode() const;
	QString getDescription() const;

	bool operator==(const QOption& other) const;
	bool operator==(const QString& other) const;

	uint getHash() const;

private:
	mutable uint m_hash;

	QString m_category;
	QString m_longCode;
	QString m_shortCode;
	QString m_desc;
	bool m_optionalArgs;
	int m_numArgs;
	QChar m_separator;
	QStringList m_argumentNames;
};

uint qHash(const QOption& option);

#endif /* QOPTION_H */

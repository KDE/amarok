/***************************************************************************
 *   This file is part of KDE.                                             *
 *   Copyright (C) 2009 Marco Martin <notmart@gmail.com>                   *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/



#include "knowledgebase.h"

using namespace Attica;

KnowledgeBase::KnowledgeBase()
    : m_id(0),
      m_contentId(0),
      m_comments(0)
{
}


void KnowledgeBase::setId(QString id)
{
    m_id = id;
}

QString KnowledgeBase::id() const
{
    return m_id;
}


void KnowledgeBase::setContentId(int id)
{
    m_contentId = id;
}

int KnowledgeBase::contentId() const
{
    return m_contentId;
}


void KnowledgeBase::setUser(const QString &user)
{
    m_user = user;
}

QString KnowledgeBase::user() const
{
    return m_user;
}


void KnowledgeBase::setStatus(const QString status)
{
    m_status = status;
}

QString KnowledgeBase::status() const
{
    return m_status;
}


void KnowledgeBase::setChanged(const QDateTime &changed)
{
    m_changed = changed;
}

QDateTime KnowledgeBase::changed() const
{
    return m_changed;
}


void KnowledgeBase::setName(const QString &name)
{
    m_name = name;
}

QString KnowledgeBase::name() const
{
    return m_name;
}


void KnowledgeBase::setDescription(const QString &description)
{
    m_description = description;
}

QString KnowledgeBase::description() const
{
    return m_description;
}


void KnowledgeBase::setAnswer(const QString &answer)
{
    m_answer = answer;
}

QString KnowledgeBase::answer() const
{
    return m_answer;
}


void KnowledgeBase::setComments(int comments)
{
    m_comments = comments;
}

int KnowledgeBase::comments() const
{
    return m_comments;
}


void KnowledgeBase::setDetailPage(const KUrl &detailPage)
{
    m_detailPage = detailPage;
}

KUrl KnowledgeBase::detailPage() const
{
    return m_detailPage;
}

void KnowledgeBase::addExtendedAttribute( const QString &key, const QString &value )
{
  m_extendedAttributes.insert( key, value );
}

QString KnowledgeBase::extendedAttribute( const QString &key ) const
{
  return m_extendedAttributes.value( key );
}

QMap<QString,QString> KnowledgeBase::extendedAttributes() const
{
  return m_extendedAttributes;
}

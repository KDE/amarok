/****************************************************************************************
* Copyright (c) 2010 Andrew Coder <andrew.coder@gmail.com                              *
*                                                                                      *
* This program is free software; you can redistribute it and/or modify it under        *
* the terms of the GNU General Public License as published by the Free Software        *
* Foundation; either version 2 of the License, or (at your option) any later           *
* version.                                                                             *
*                                                                                      *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
* PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
*                                                                                      *
* You should have received a copy of the GNU General Public License along with         *
* this program.  If not, see <http://www.gnu.org/licenses/>.                           *
****************************************************************************************/

#ifndef QUERYMAKER_FUNCTION_TYPES_H
#define QUERYMAKER_FUNCTION_TYPES_H

#include "core/collections/QueryMaker.h"

using namespace Collections;

class CurriedQMFunction
{
    public:
        virtual ~CurriedQMFunction() {};
        virtual QueryMaker* operator()( QueryMaker *qm = 0 ) = 0;
};

class CurriedZeroArityQMFunction : public CurriedQMFunction
{
    public:
        typedef QueryMaker* ( QueryMaker::*FunPtr ) ();
        
        CurriedZeroArityQMFunction( FunPtr function )
            : m_function( function )
        {};
        virtual ~CurriedZeroArityQMFunction() {};
        
        QueryMaker* operator()( QueryMaker *qm = 0 )
        {
            if( qm )
                return ( qm->*m_function )();
            return qm;
        };
        
    private:
        FunPtr m_function;
};

template< class Type >
class CurriedUnaryQMFunction : public CurriedQMFunction
{    
    public:
        typedef QueryMaker* ( QueryMaker::*FunPtr ) ( Type );
        
        CurriedUnaryQMFunction( FunPtr function, Type parameter )
            : m_function( function )
            , m_parameter( parameter )
        {};
        virtual ~CurriedUnaryQMFunction() {};
        
        QueryMaker* operator()( QueryMaker *qm )
        {
            if( qm )
                return ( qm->*m_function )( m_parameter );
            return qm;
        };
        
    private:
        FunPtr m_function;
        Type m_parameter;
};

template< class FirstType, class SecondType >
class CurriedBinaryQMFunction : public CurriedQMFunction
{
    public:
        typedef QueryMaker* ( QueryMaker::*FunPtr ) ( FirstType, SecondType );
        
        CurriedBinaryQMFunction( FunPtr function, FirstType parameterOne, SecondType parameterTwo )
            : m_function( function )
            , m_parameterOne( parameterOne )
            , m_parameterTwo( parameterTwo )
        {};
        virtual ~CurriedBinaryQMFunction() {};
        
        QueryMaker* operator()( QueryMaker *qm )
        {
            if( qm )
                return ( qm->*m_function )( m_parameterOne, m_parameterTwo );
            return qm;
        };
        
    private:
        FunPtr m_function;
        FirstType m_parameterOne;
        SecondType m_parameterTwo;
};

template< class FirstType, class SecondType, class ThirdType >
class CurriedTrinaryQMFunction : public CurriedQMFunction
{
    public:
        typedef QueryMaker* ( QueryMaker::*FunPtr ) ( FirstType, SecondType, ThirdType );
        
        CurriedTrinaryQMFunction( FunPtr function, FirstType parameterOne, SecondType parameterTwo, ThirdType parameterThree )
            : m_function( function )
            , m_parameterOne( parameterOne )
            , m_parameterTwo( parameterTwo )
            , m_parameterThree( parameterThree )
        {};
        virtual ~CurriedTrinaryQMFunction() {};
        
        QueryMaker* operator()( QueryMaker *qm )
        {
            if( qm )
                return ( qm->*m_function )( m_parameterOne, m_parameterTwo, m_parameterThree );
            return qm;
        };
        
    private:
        FunPtr m_function;
        FirstType m_parameterOne;
        SecondType m_parameterTwo;
        ThirdType m_parameterThree;
};

class CurriedQMStringFilterFunction : public CurriedQMFunction
{
    public:
        typedef QueryMaker* ( QueryMaker::*FunPtr ) ( qint64, const QString&, bool, bool );
        
        CurriedQMStringFilterFunction( FunPtr function, qint64 value, QString filter, bool matchBegin, bool matchEnd )
            : m_function( function )
            , m_value( value )
            , m_filter( filter )
            , m_matchBegin( matchBegin )
            , m_matchEnd( matchEnd )
        {};
        virtual ~CurriedQMStringFilterFunction() {};

        QueryMaker* operator()( QueryMaker *qm )
        {
            if( qm )
                return ( qm->*m_function )( m_value, m_filter, m_matchBegin, m_matchEnd );
            return qm;
        };
        
    private:
        FunPtr m_function;
        qint64 m_value;
        QString m_filter;
        bool m_matchBegin;
        bool m_matchEnd;
};

#endif
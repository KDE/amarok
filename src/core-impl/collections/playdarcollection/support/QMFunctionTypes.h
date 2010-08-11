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
        virtual QueryMaker* operator()() = 0;
};

class CurriedZeroArityQMFunction : public CurriedQMFunction
{
    public:
        typedef QueryMaker* ( QueryMaker::*FunPtr ) ();
        
        CurriedZeroArityQMFunction( QueryMaker *qm, FunPtr function )
            : m_queryMaker( qm )
            , m_function( function )
        {};
        
        QueryMaker* operator()()
        {
            ( m_queryMaker->*m_function )();
            return m_queryMaker;
        };
        
    private:
        QueryMaker* m_queryMaker;
        FunPtr m_function;
};

template< class Type >
class CurriedUnaryQMFunction : public CurriedQMFunction
{    
    public:
        typedef QueryMaker* ( QueryMaker::*FunPtr ) ( Type );
        
        CurriedUnaryQMFunction( QueryMaker *qm, FunPtr function, Type parameter )
            : m_queryMaker( qm )
            , m_function( function )
            , m_parameter( parameter )
        {};
        
        QueryMaker* operator()()
        {
            ( m_queryMaker->*m_function )( m_parameter );
            return m_queryMaker;
        };
        
    private:
        QueryMaker* m_queryMaker;
        FunPtr m_function;
        Type m_parameter;
};

template< class FirstType, class SecondType >
class CurriedBinaryQMFunction : public CurriedQMFunction
{
    public:
        typedef QueryMaker* ( QueryMaker::*FunPtr ) ( FirstType, SecondType );
        
        CurriedBinaryQMFunction( QueryMaker *qm, FunPtr function, FirstType parameterOne, SecondType parameterTwo )
            : m_queryMaker( qm )
            , m_function( function )
            , m_parameterOne( parameterOne )
            , m_parameterTwo( parameterTwo )
        {};
        
        QueryMaker* operator()()
        {
            ( m_queryMaker->*m_function )( m_parameterOne, m_parameterTwo );
            return m_queryMaker;
        };
        
    private:
        QueryMaker* m_queryMaker;
        FunPtr m_function;
        FirstType m_parameterOne;
        SecondType m_parameterTwo;
};

template< class FirstType, class SecondType, class ThirdType >
class CurriedTrinaryQMFunction : public CurriedQMFunction
{
    public:
        typedef QueryMaker* ( QueryMaker::*FunPtr ) ( FirstType, SecondType, ThirdType );
        
        CurriedTrinaryQMFunction( QueryMaker *qm, FunPtr function, FirstType parameterOne, SecondType parameterTwo, ThirdType parameterThree )
            : m_queryMaker( qm )
            , m_function( function )
            , m_parameterOne( parameterOne )
            , m_parameterTwo( parameterTwo )
            , m_parameterThree( parameterThree )
        {};
        
        QueryMaker* operator()()
        {
            ( m_queryMaker->*m_function )( m_parameterOne, m_parameterTwo, m_parameterThree );
            return m_queryMaker;
        };
        
    private:
        QueryMaker* m_queryMaker;
        FunPtr m_function;
        FirstType m_parameterOne;
        SecondType m_parameterTwo;
        ThirdType m_parameterThree;
};

template< class FirstType, class SecondType, class ThirdType, class FourthType >
class CurriedQuarternaryQMFunction : public CurriedQMFunction
{
    public:
        typedef QueryMaker* ( QueryMaker::*FunPtr ) ( FirstType, SecondType, ThirdType, FourthType );
        
        CurriedQuarternaryQMFunction( QueryMaker *qm, FunPtr function, FirstType parameterOne, SecondType parameterTwo, ThirdType parameterThree, FourthType parameterFour )
            : m_queryMaker( qm )
            , m_function( function )
            , m_parameterOne( parameterOne )
            , m_parameterTwo( parameterTwo )
            , m_parameterThree( parameterThree )
            , m_parameterFour( parameterFour )
        {};

        QueryMaker* operator()()
        {
            ( m_queryMaker->*m_function )( m_parameterOne, m_parameterTwo, m_parameterThree, m_parameterFour );
            return m_queryMaker;
        };
        
    private:
        QueryMaker* m_queryMaker;
        FunPtr m_function;
        FirstType m_parameterOne;
        SecondType m_parameterTwo;
        ThirdType m_parameterThree;
        FourthType m_parameterFour;
};

#endif
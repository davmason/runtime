// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

using System;
public class BaseNode
{
    int iValue = 0;
    int iType = 111111;

    static bool UseFinals = true;


    public BaseNode()
    {
    }

    public static bool getUseFinal()
    {
        return UseFinals;
    }

    public static void setUseFinal( bool Final )
    {
        UseFinals = Final;
    }

    public void setValue( int Value )
    {
        iValue = Value;
    }

    public int getValue()
    {
        return iValue;
    }
    
    public void setType( int Type )
    {
        iType = Type;
    }

    public int getType()
    {
        return iType;
    }
    
    public void finalize()
    {
        if ( getUseFinal() )
            Console.WriteLine( "Finalized Random Node: " + getValue() );
    }
}

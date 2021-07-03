// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

public class RandomNode : /*extends*/ BaseNode
{
    int iiSize = 0;    
	byte[] SimpleSize;
	static int iSize = 0;
	

	public RandomNode( int Size, int value )
	{
        setType( 2 );
		setValue( value );
		SimpleSize = new byte[Size];	
		

		if ( Size != 0 ) 
		{
			SimpleSize[0] = (byte)255;
			SimpleSize[(Size - 1)] = (byte)255;
		}

		setSize( Size );

	}
	
	public void setSize( int Size )
	{
		iiSize = Size;
		iSize += Size;
	}

	public int getSize( )
	{
		return iiSize;
	}

	public static int getBigSize( )
	{
		return iSize;
	}

	public static void setBigSize( int Size )
	{
		iSize = Size;
	}
}
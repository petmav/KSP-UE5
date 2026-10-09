using System.Reflection.Emit;

namespace System.Xml.Serialization;

internal class IfState
{
	private Label elseBegin;

	private Label endIf;

	internal Label EndIf
	{
		get
		{
			return endIf;
		}
		set
		{
			endIf = value;
		}
	}

	internal Label ElseBegin
	{
		get
		{
			return elseBegin;
		}
		set
		{
			elseBegin = value;
		}
	}
}

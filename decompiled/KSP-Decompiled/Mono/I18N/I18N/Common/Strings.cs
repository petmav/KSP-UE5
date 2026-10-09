using System;

namespace I18N.Common;

public sealed class Strings
{
	public static string GetString(string tag)
	{
		return tag switch
		{
			"ArgRange_Array" => "Argument index is out of array range.", 
			"Arg_InsufficientSpace" => "Insufficient space in the argument array.", 
			"ArgRange_NonNegative" => "Non-negative value is expected.", 
			"NotSupp_MissingCodeTable" => "This encoding is not supported. Code table is missing.", 
			"ArgRange_StringIndex" => "String index is out of range.", 
			"ArgRange_StringRange" => "String length is out of range.", 
			_ => throw new ArgumentException($"Unexpected error tag name:  {tag}"), 
		};
	}
}

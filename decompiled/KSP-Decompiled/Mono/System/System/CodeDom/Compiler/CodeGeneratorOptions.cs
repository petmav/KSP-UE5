using System.Collections;
using System.Collections.Specialized;

namespace System.CodeDom.Compiler;

public class CodeGeneratorOptions
{
	private readonly IDictionary _options = new ListDictionary();

	public object this[string index]
	{
		get
		{
			return _options[index];
		}
		set
		{
			_options[index] = value;
		}
	}

	public string IndentString
	{
		get
		{
			object obj = _options["IndentString"];
			if (obj == null)
			{
				return "    ";
			}
			return (string)obj;
		}
		set
		{
			_options["IndentString"] = value;
		}
	}

	public string BracingStyle
	{
		get
		{
			object obj = _options["BracingStyle"];
			if (obj == null)
			{
				return "Block";
			}
			return (string)obj;
		}
		set
		{
			_options["BracingStyle"] = value;
		}
	}

	public bool ElseOnClosing
	{
		get
		{
			object obj = _options["ElseOnClosing"];
			if (obj == null)
			{
				return false;
			}
			return (bool)obj;
		}
		set
		{
			_options["ElseOnClosing"] = value;
		}
	}

	public bool BlankLinesBetweenMembers
	{
		get
		{
			object obj = _options["BlankLinesBetweenMembers"];
			if (obj == null)
			{
				return true;
			}
			return (bool)obj;
		}
		set
		{
			_options["BlankLinesBetweenMembers"] = value;
		}
	}

	public bool VerbatimOrder
	{
		get
		{
			object obj = _options["VerbatimOrder"];
			if (obj == null)
			{
				return false;
			}
			return (bool)obj;
		}
		set
		{
			_options["VerbatimOrder"] = value;
		}
	}
}

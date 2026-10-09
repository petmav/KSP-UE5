using System.Collections.Generic;

namespace System.Net.Http.Headers;

public class ProductHeaderValue : ICloneable
{
	public string Name { get; internal set; }

	public string Version { get; internal set; }

	public ProductHeaderValue(string name)
	{
		Parser.Token.Check(name);
		Name = name;
	}

	public ProductHeaderValue(string name, string version)
		: this(name)
	{
		if (version != null)
		{
			Parser.Token.Check(version);
		}
		Version = version;
	}

	internal ProductHeaderValue()
	{
	}

	object ICloneable.Clone()
	{
		return MemberwiseClone();
	}

	public override bool Equals(object obj)
	{
		if (!(obj is ProductHeaderValue productHeaderValue))
		{
			return false;
		}
		if (string.Equals(productHeaderValue.Name, Name, StringComparison.OrdinalIgnoreCase))
		{
			return string.Equals(productHeaderValue.Version, Version, StringComparison.OrdinalIgnoreCase);
		}
		return false;
	}

	public override int GetHashCode()
	{
		int num = Name.ToLowerInvariant().GetHashCode();
		if (Version != null)
		{
			num ^= Version.ToLowerInvariant().GetHashCode();
		}
		return num;
	}

	public static ProductHeaderValue Parse(string input)
	{
		if (TryParse(input, out var parsedValue))
		{
			return parsedValue;
		}
		throw new FormatException(input);
	}

	public static bool TryParse(string input, out ProductHeaderValue parsedValue)
	{
		if (TryParseElement(new Lexer(input), out parsedValue, out var t) && (Token.Type)t == Token.Type.End)
		{
			return true;
		}
		parsedValue = null;
		return false;
	}

	internal static bool TryParse(string input, int minimalCount, out List<ProductHeaderValue> result)
	{
		return CollectionParser.TryParse(input, minimalCount, (ElementTryParser<ProductHeaderValue>)TryParseElement, out result);
	}

	private static bool TryParseElement(Lexer lexer, out ProductHeaderValue parsedValue, out Token t)
	{
		parsedValue = null;
		t = lexer.Scan();
		if ((Token.Type)t != Token.Type.Token)
		{
			return false;
		}
		parsedValue = new ProductHeaderValue();
		parsedValue.Name = lexer.GetStringValue(t);
		t = lexer.Scan();
		if ((Token.Type)t == Token.Type.SeparatorSlash)
		{
			t = lexer.Scan();
			if ((Token.Type)t != Token.Type.Token)
			{
				return false;
			}
			parsedValue.Version = lexer.GetStringValue(t);
			t = lexer.Scan();
		}
		return true;
	}

	public override string ToString()
	{
		if (Version != null)
		{
			return Name + "/" + Version;
		}
		return Name;
	}
}

using System.Collections.Generic;

namespace System.Net.Http.Headers;

public class NameValueHeaderValue : ICloneable
{
	internal string value;

	public string Name { get; internal set; }

	public string Value
	{
		get
		{
			return value;
		}
		set
		{
			if (!string.IsNullOrEmpty(value))
			{
				Lexer lexer = new Lexer(value);
				Token token = lexer.Scan();
				if ((Token.Type)lexer.Scan() != Token.Type.End || ((Token.Type)token != Token.Type.Token && (Token.Type)token != Token.Type.QuotedString))
				{
					throw new FormatException();
				}
				value = lexer.GetStringValue(token);
			}
			this.value = value;
		}
	}

	public NameValueHeaderValue(string name)
		: this(name, null)
	{
	}

	public NameValueHeaderValue(string name, string value)
	{
		Parser.Token.Check(name);
		Name = name;
		Value = value;
	}

	protected internal NameValueHeaderValue(NameValueHeaderValue source)
	{
		Name = source.Name;
		value = source.value;
	}

	internal NameValueHeaderValue()
	{
	}

	internal static NameValueHeaderValue Create(string name, string value)
	{
		return new NameValueHeaderValue
		{
			Name = name,
			value = value
		};
	}

	object ICloneable.Clone()
	{
		return new NameValueHeaderValue(this);
	}

	public override int GetHashCode()
	{
		int num = Name.ToLowerInvariant().GetHashCode();
		if (!string.IsNullOrEmpty(value))
		{
			num ^= value.ToLowerInvariant().GetHashCode();
		}
		return num;
	}

	public override bool Equals(object obj)
	{
		if (!(obj is NameValueHeaderValue nameValueHeaderValue) || !string.Equals(nameValueHeaderValue.Name, Name, StringComparison.OrdinalIgnoreCase))
		{
			return false;
		}
		if (string.IsNullOrEmpty(value))
		{
			return string.IsNullOrEmpty(nameValueHeaderValue.value);
		}
		return string.Equals(nameValueHeaderValue.value, value, StringComparison.OrdinalIgnoreCase);
	}

	public static NameValueHeaderValue Parse(string input)
	{
		if (TryParse(input, out var parsedValue))
		{
			return parsedValue;
		}
		throw new FormatException(input);
	}

	internal static bool TryParsePragma(string input, int minimalCount, out List<NameValueHeaderValue> result)
	{
		return CollectionParser.TryParse(input, minimalCount, (ElementTryParser<NameValueHeaderValue>)TryParseElement, out result);
	}

	internal static bool TryParseParameters(Lexer lexer, out List<NameValueHeaderValue> result, out Token t)
	{
		List<NameValueHeaderValue> list = new List<NameValueHeaderValue>();
		result = null;
		do
		{
			Token token = lexer.Scan();
			if ((Token.Type)token != Token.Type.Token)
			{
				t = Token.Empty;
				return false;
			}
			string text = null;
			t = lexer.Scan();
			if ((Token.Type)t == Token.Type.SeparatorEqual)
			{
				t = lexer.Scan();
				if ((Token.Type)t != Token.Type.Token && (Token.Type)t != Token.Type.QuotedString)
				{
					return false;
				}
				text = lexer.GetStringValue(t);
				t = lexer.Scan();
			}
			list.Add(new NameValueHeaderValue
			{
				Name = lexer.GetStringValue(token),
				value = text
			});
		}
		while ((Token.Type)t == Token.Type.SeparatorSemicolon);
		result = list;
		return true;
	}

	public override string ToString()
	{
		if (string.IsNullOrEmpty(value))
		{
			return Name;
		}
		return Name + "=" + value;
	}

	public static bool TryParse(string input, out NameValueHeaderValue parsedValue)
	{
		if (TryParseElement(new Lexer(input), out parsedValue, out var t) && (Token.Type)t == Token.Type.End)
		{
			return true;
		}
		parsedValue = null;
		return false;
	}

	private static bool TryParseElement(Lexer lexer, out NameValueHeaderValue parsedValue, out Token t)
	{
		parsedValue = null;
		t = lexer.Scan();
		if ((Token.Type)t != Token.Type.Token)
		{
			return false;
		}
		parsedValue = new NameValueHeaderValue
		{
			Name = lexer.GetStringValue(t)
		};
		t = lexer.Scan();
		if ((Token.Type)t == Token.Type.SeparatorEqual)
		{
			t = lexer.Scan();
			if ((Token.Type)t != Token.Type.Token && (Token.Type)t != Token.Type.QuotedString)
			{
				return false;
			}
			parsedValue.value = lexer.GetStringValue(t);
			t = lexer.Scan();
		}
		return true;
	}
}

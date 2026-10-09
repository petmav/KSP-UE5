using System.Collections.Generic;
using System.Globalization;

namespace System.Net.Http.Headers;

public class StringWithQualityHeaderValue : ICloneable
{
	public double? Quality { get; private set; }

	public string Value { get; private set; }

	public StringWithQualityHeaderValue(string value)
	{
		Parser.Token.Check(value);
		Value = value;
	}

	public StringWithQualityHeaderValue(string value, double quality)
		: this(value)
	{
		if (quality < 0.0 || quality > 1.0)
		{
			throw new ArgumentOutOfRangeException("quality");
		}
		Quality = quality;
	}

	private StringWithQualityHeaderValue()
	{
	}

	object ICloneable.Clone()
	{
		return MemberwiseClone();
	}

	public override bool Equals(object obj)
	{
		if (obj is StringWithQualityHeaderValue stringWithQualityHeaderValue && string.Equals(stringWithQualityHeaderValue.Value, Value, StringComparison.OrdinalIgnoreCase))
		{
			return stringWithQualityHeaderValue.Quality == Quality;
		}
		return false;
	}

	public override int GetHashCode()
	{
		return Value.ToLowerInvariant().GetHashCode() ^ Quality.GetHashCode();
	}

	public static StringWithQualityHeaderValue Parse(string input)
	{
		if (TryParse(input, out var parsedValue))
		{
			return parsedValue;
		}
		throw new FormatException(input);
	}

	public static bool TryParse(string input, out StringWithQualityHeaderValue parsedValue)
	{
		if (TryParseElement(new Lexer(input), out parsedValue, out var t) && (Token.Type)t == Token.Type.End)
		{
			return true;
		}
		parsedValue = null;
		return false;
	}

	internal static bool TryParse(string input, int minimalCount, out List<StringWithQualityHeaderValue> result)
	{
		return CollectionParser.TryParse(input, minimalCount, (ElementTryParser<StringWithQualityHeaderValue>)TryParseElement, out result);
	}

	private static bool TryParseElement(Lexer lexer, out StringWithQualityHeaderValue parsedValue, out Token t)
	{
		parsedValue = null;
		t = lexer.Scan();
		if ((Token.Type)t != Token.Type.Token)
		{
			return false;
		}
		StringWithQualityHeaderValue stringWithQualityHeaderValue = new StringWithQualityHeaderValue();
		stringWithQualityHeaderValue.Value = lexer.GetStringValue(t);
		t = lexer.Scan();
		if ((Token.Type)t == Token.Type.SeparatorSemicolon)
		{
			t = lexer.Scan();
			if ((Token.Type)t != Token.Type.Token)
			{
				return false;
			}
			string stringValue = lexer.GetStringValue(t);
			if (stringValue != "q" && stringValue != "Q")
			{
				return false;
			}
			t = lexer.Scan();
			if ((Token.Type)t != Token.Type.SeparatorEqual)
			{
				return false;
			}
			t = lexer.Scan();
			if (!lexer.TryGetDoubleValue(t, out var value))
			{
				return false;
			}
			if (value > 1.0)
			{
				return false;
			}
			stringWithQualityHeaderValue.Quality = value;
			t = lexer.Scan();
		}
		parsedValue = stringWithQualityHeaderValue;
		return true;
	}

	public override string ToString()
	{
		if (Quality.HasValue)
		{
			return Value + "; q=" + Quality.Value.ToString("0.0##", CultureInfo.InvariantCulture);
		}
		return Value;
	}
}

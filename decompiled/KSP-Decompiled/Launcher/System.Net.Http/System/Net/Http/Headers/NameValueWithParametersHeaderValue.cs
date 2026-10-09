using System.Collections.Generic;

namespace System.Net.Http.Headers;

public class NameValueWithParametersHeaderValue : NameValueHeaderValue, ICloneable
{
	private List<NameValueHeaderValue> parameters;

	public ICollection<NameValueHeaderValue> Parameters => parameters ?? (parameters = new List<NameValueHeaderValue>());

	public NameValueWithParametersHeaderValue(string name)
		: base(name)
	{
	}

	public NameValueWithParametersHeaderValue(string name, string value)
		: base(name, value)
	{
	}

	protected NameValueWithParametersHeaderValue(NameValueWithParametersHeaderValue source)
		: base(source)
	{
		if (source.parameters == null)
		{
			return;
		}
		foreach (NameValueHeaderValue parameter in source.parameters)
		{
			Parameters.Add(parameter);
		}
	}

	private NameValueWithParametersHeaderValue()
	{
	}

	object ICloneable.Clone()
	{
		return new NameValueWithParametersHeaderValue(this);
	}

	public override bool Equals(object obj)
	{
		if (!(obj is NameValueWithParametersHeaderValue nameValueWithParametersHeaderValue))
		{
			return false;
		}
		if (base.Equals(obj))
		{
			return nameValueWithParametersHeaderValue.parameters.SequenceEqual(parameters);
		}
		return false;
	}

	public override int GetHashCode()
	{
		return base.GetHashCode() ^ HashCodeCalculator.Calculate(parameters);
	}

	public new static NameValueWithParametersHeaderValue Parse(string input)
	{
		if (TryParse(input, out var parsedValue))
		{
			return parsedValue;
		}
		throw new FormatException(input);
	}

	public override string ToString()
	{
		if (parameters == null || parameters.Count == 0)
		{
			return base.ToString();
		}
		return base.ToString() + CollectionExtensions.ToString(parameters);
	}

	public static bool TryParse(string input, out NameValueWithParametersHeaderValue parsedValue)
	{
		if (TryParseElement(new Lexer(input), out parsedValue, out var t) && (Token.Type)t == Token.Type.End)
		{
			return true;
		}
		parsedValue = null;
		return false;
	}

	internal static bool TryParse(string input, int minimalCount, out List<NameValueWithParametersHeaderValue> result)
	{
		return CollectionParser.TryParse(input, minimalCount, (ElementTryParser<NameValueWithParametersHeaderValue>)TryParseElement, out result);
	}

	private static bool TryParseElement(Lexer lexer, out NameValueWithParametersHeaderValue parsedValue, out Token t)
	{
		parsedValue = null;
		t = lexer.Scan();
		if ((Token.Type)t != Token.Type.Token)
		{
			return false;
		}
		parsedValue = new NameValueWithParametersHeaderValue
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
		if ((Token.Type)t == Token.Type.SeparatorSemicolon)
		{
			if (!NameValueHeaderValue.TryParseParameters(lexer, out var result, out t))
			{
				return false;
			}
			parsedValue.parameters = result;
		}
		return true;
	}
}

using System.Collections.Generic;

namespace System.Net.Http.Headers;

public class AuthenticationHeaderValue : ICloneable
{
	public string Parameter { get; private set; }

	public string Scheme { get; private set; }

	public AuthenticationHeaderValue(string scheme)
		: this(scheme, null)
	{
	}

	public AuthenticationHeaderValue(string scheme, string parameter)
	{
		Parser.Token.Check(scheme);
		Scheme = scheme;
		Parameter = parameter;
	}

	private AuthenticationHeaderValue()
	{
	}

	object ICloneable.Clone()
	{
		return MemberwiseClone();
	}

	public override bool Equals(object obj)
	{
		if (obj is AuthenticationHeaderValue authenticationHeaderValue && string.Equals(authenticationHeaderValue.Scheme, Scheme, StringComparison.OrdinalIgnoreCase))
		{
			return authenticationHeaderValue.Parameter == Parameter;
		}
		return false;
	}

	public override int GetHashCode()
	{
		int num = Scheme.ToLowerInvariant().GetHashCode();
		if (!string.IsNullOrEmpty(Parameter))
		{
			num ^= Parameter.ToLowerInvariant().GetHashCode();
		}
		return num;
	}

	public static AuthenticationHeaderValue Parse(string input)
	{
		if (TryParse(input, out var parsedValue))
		{
			return parsedValue;
		}
		throw new FormatException(input);
	}

	public static bool TryParse(string input, out AuthenticationHeaderValue parsedValue)
	{
		if (TryParseElement(new Lexer(input), out parsedValue, out var t) && (Token.Type)t == Token.Type.End)
		{
			return true;
		}
		parsedValue = null;
		return false;
	}

	internal static bool TryParse(string input, int minimalCount, out List<AuthenticationHeaderValue> result)
	{
		return CollectionParser.TryParse(input, minimalCount, (ElementTryParser<AuthenticationHeaderValue>)TryParseElement, out result);
	}

	private static bool TryParseElement(Lexer lexer, out AuthenticationHeaderValue parsedValue, out Token t)
	{
		t = lexer.Scan();
		if ((Token.Type)t != Token.Type.Token)
		{
			parsedValue = null;
			return false;
		}
		parsedValue = new AuthenticationHeaderValue();
		parsedValue.Scheme = lexer.GetStringValue(t);
		t = lexer.Scan();
		if ((Token.Type)t == Token.Type.Token)
		{
			parsedValue.Parameter = lexer.GetRemainingStringValue(t.StartPosition);
			t = new Token(Token.Type.End, 0, 0);
		}
		return true;
	}

	public override string ToString()
	{
		if (Parameter == null)
		{
			return Scheme;
		}
		return Scheme + " " + Parameter;
	}
}

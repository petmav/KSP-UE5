using System.Collections.Generic;
using System.Globalization;

namespace System.Net.Http.Headers;

public class WarningHeaderValue : ICloneable
{
	public string Agent { get; private set; }

	public int Code { get; private set; }

	public DateTimeOffset? Date { get; private set; }

	public string Text { get; private set; }

	public WarningHeaderValue(int code, string agent, string text)
	{
		if (!IsCodeValid(code))
		{
			throw new ArgumentOutOfRangeException("code");
		}
		Parser.Uri.Check(agent);
		Parser.Token.CheckQuotedString(text);
		Code = code;
		Agent = agent;
		Text = text;
	}

	public WarningHeaderValue(int code, string agent, string text, DateTimeOffset date)
		: this(code, agent, text)
	{
		Date = date;
	}

	private WarningHeaderValue()
	{
	}

	private static bool IsCodeValid(int code)
	{
		if (code >= 0)
		{
			return code < 1000;
		}
		return false;
	}

	object ICloneable.Clone()
	{
		return MemberwiseClone();
	}

	public override bool Equals(object obj)
	{
		if (!(obj is WarningHeaderValue warningHeaderValue))
		{
			return false;
		}
		if (Code == warningHeaderValue.Code && string.Equals(warningHeaderValue.Agent, Agent, StringComparison.OrdinalIgnoreCase) && Text == warningHeaderValue.Text)
		{
			DateTimeOffset? date = Date;
			DateTimeOffset? date2 = warningHeaderValue.Date;
			if (date.HasValue != date2.HasValue)
			{
				return false;
			}
			if (!date.HasValue)
			{
				return true;
			}
			return date.GetValueOrDefault() == date2.GetValueOrDefault();
		}
		return false;
	}

	public override int GetHashCode()
	{
		return Code.GetHashCode() ^ Agent.ToLowerInvariant().GetHashCode() ^ Text.GetHashCode() ^ Date.GetHashCode();
	}

	public static WarningHeaderValue Parse(string input)
	{
		if (TryParse(input, out var parsedValue))
		{
			return parsedValue;
		}
		throw new FormatException(input);
	}

	public static bool TryParse(string input, out WarningHeaderValue parsedValue)
	{
		if (TryParseElement(new Lexer(input), out parsedValue, out var t) && (Token.Type)t == Token.Type.End)
		{
			return true;
		}
		parsedValue = null;
		return false;
	}

	internal static bool TryParse(string input, int minimalCount, out List<WarningHeaderValue> result)
	{
		return CollectionParser.TryParse(input, minimalCount, (ElementTryParser<WarningHeaderValue>)TryParseElement, out result);
	}

	private static bool TryParseElement(Lexer lexer, out WarningHeaderValue parsedValue, out Token t)
	{
		parsedValue = null;
		t = lexer.Scan();
		if ((Token.Type)t != Token.Type.Token)
		{
			return false;
		}
		if (!lexer.TryGetNumericValue(t, out int value) || !IsCodeValid(value))
		{
			return false;
		}
		t = lexer.Scan();
		if ((Token.Type)t != Token.Type.Token)
		{
			return false;
		}
		Token token = t;
		if (lexer.PeekChar() == 58)
		{
			lexer.EatChar();
			token = lexer.Scan();
			if ((Token.Type)token != Token.Type.Token)
			{
				return false;
			}
		}
		WarningHeaderValue warningHeaderValue = new WarningHeaderValue();
		warningHeaderValue.Code = value;
		warningHeaderValue.Agent = lexer.GetStringValue(t, token);
		t = lexer.Scan();
		if ((Token.Type)t != Token.Type.QuotedString)
		{
			return false;
		}
		warningHeaderValue.Text = lexer.GetStringValue(t);
		t = lexer.Scan();
		if ((Token.Type)t == Token.Type.QuotedString)
		{
			if (!lexer.TryGetDateValue(t, out var value2))
			{
				return false;
			}
			warningHeaderValue.Date = value2;
			t = lexer.Scan();
		}
		parsedValue = warningHeaderValue;
		return true;
	}

	public override string ToString()
	{
		string text = Code.ToString("000") + " " + Agent + " " + Text;
		if (Date.HasValue)
		{
			text = text + " \"" + Date.Value.ToString("r", CultureInfo.InvariantCulture) + "\"";
		}
		return text;
	}
}

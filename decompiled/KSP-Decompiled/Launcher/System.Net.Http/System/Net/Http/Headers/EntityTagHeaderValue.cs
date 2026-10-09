using System.Collections.Generic;

namespace System.Net.Http.Headers;

public class EntityTagHeaderValue : ICloneable
{
	private static readonly EntityTagHeaderValue any = new EntityTagHeaderValue
	{
		Tag = "*"
	};

	public static EntityTagHeaderValue Any => any;

	public bool IsWeak { get; internal set; }

	public string Tag { get; internal set; }

	public EntityTagHeaderValue(string tag)
	{
		Parser.Token.CheckQuotedString(tag);
		Tag = tag;
	}

	public EntityTagHeaderValue(string tag, bool isWeak)
		: this(tag)
	{
		IsWeak = isWeak;
	}

	internal EntityTagHeaderValue()
	{
	}

	object ICloneable.Clone()
	{
		return MemberwiseClone();
	}

	public override bool Equals(object obj)
	{
		if (obj is EntityTagHeaderValue entityTagHeaderValue && entityTagHeaderValue.Tag == Tag)
		{
			return string.Equals(entityTagHeaderValue.Tag, Tag, StringComparison.Ordinal);
		}
		return false;
	}

	public override int GetHashCode()
	{
		return IsWeak.GetHashCode() ^ Tag.GetHashCode();
	}

	public static EntityTagHeaderValue Parse(string input)
	{
		if (TryParse(input, out var parsedValue))
		{
			return parsedValue;
		}
		throw new FormatException(input);
	}

	public static bool TryParse(string input, out EntityTagHeaderValue parsedValue)
	{
		if (TryParseElement(new Lexer(input), out parsedValue, out var t) && (Token.Type)t == Token.Type.End)
		{
			return true;
		}
		parsedValue = null;
		return false;
	}

	private static bool TryParseElement(Lexer lexer, out EntityTagHeaderValue parsedValue, out Token t)
	{
		parsedValue = null;
		t = lexer.Scan();
		bool isWeak = false;
		if ((Token.Type)t == Token.Type.Token)
		{
			string stringValue = lexer.GetStringValue(t);
			if (stringValue == "*")
			{
				parsedValue = any;
				t = lexer.Scan();
				return true;
			}
			if (stringValue != "W" || lexer.PeekChar() != 47)
			{
				return false;
			}
			isWeak = true;
			lexer.EatChar();
			t = lexer.Scan();
		}
		if ((Token.Type)t != Token.Type.QuotedString)
		{
			return false;
		}
		parsedValue = new EntityTagHeaderValue();
		parsedValue.Tag = lexer.GetStringValue(t);
		parsedValue.IsWeak = isWeak;
		t = lexer.Scan();
		return true;
	}

	internal static bool TryParse(string input, int minimalCount, out List<EntityTagHeaderValue> result)
	{
		return CollectionParser.TryParse(input, minimalCount, (ElementTryParser<EntityTagHeaderValue>)TryParseElement, out result);
	}

	public override string ToString()
	{
		if (!IsWeak)
		{
			return Tag;
		}
		return "W/" + Tag;
	}
}

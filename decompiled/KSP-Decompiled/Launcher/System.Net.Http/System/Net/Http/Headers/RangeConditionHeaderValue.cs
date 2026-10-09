using System.Globalization;

namespace System.Net.Http.Headers;

public class RangeConditionHeaderValue : ICloneable
{
	public DateTimeOffset? Date { get; private set; }

	public EntityTagHeaderValue EntityTag { get; private set; }

	public RangeConditionHeaderValue(DateTimeOffset date)
	{
		Date = date;
	}

	public RangeConditionHeaderValue(EntityTagHeaderValue entityTag)
	{
		if (entityTag == null)
		{
			throw new ArgumentNullException("entityTag");
		}
		EntityTag = entityTag;
	}

	public RangeConditionHeaderValue(string entityTag)
		: this(new EntityTagHeaderValue(entityTag))
	{
	}

	object ICloneable.Clone()
	{
		return MemberwiseClone();
	}

	public override bool Equals(object obj)
	{
		if (!(obj is RangeConditionHeaderValue rangeConditionHeaderValue))
		{
			return false;
		}
		if (EntityTag == null)
		{
			DateTimeOffset? date = Date;
			DateTimeOffset? date2 = rangeConditionHeaderValue.Date;
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
		return EntityTag.Equals(rangeConditionHeaderValue.EntityTag);
	}

	public override int GetHashCode()
	{
		if (EntityTag == null)
		{
			return Date.GetHashCode();
		}
		return EntityTag.GetHashCode();
	}

	public static RangeConditionHeaderValue Parse(string input)
	{
		if (TryParse(input, out var parsedValue))
		{
			return parsedValue;
		}
		throw new FormatException(input);
	}

	public static bool TryParse(string input, out RangeConditionHeaderValue parsedValue)
	{
		parsedValue = null;
		Lexer lexer = new Lexer(input);
		Token token = lexer.Scan();
		bool isWeak;
		if ((Token.Type)token == Token.Type.Token)
		{
			if (lexer.GetStringValue(token) != "W")
			{
				if (!Lexer.TryGetDateValue(input, out var value))
				{
					return false;
				}
				parsedValue = new RangeConditionHeaderValue(value);
				return true;
			}
			if (lexer.PeekChar() != 47)
			{
				return false;
			}
			isWeak = true;
			lexer.EatChar();
			token = lexer.Scan();
		}
		else
		{
			isWeak = false;
		}
		if ((Token.Type)token != Token.Type.QuotedString)
		{
			return false;
		}
		if ((Token.Type)lexer.Scan() != Token.Type.End)
		{
			return false;
		}
		parsedValue = new RangeConditionHeaderValue(new EntityTagHeaderValue
		{
			Tag = lexer.GetStringValue(token),
			IsWeak = isWeak
		});
		return true;
	}

	public override string ToString()
	{
		if (EntityTag != null)
		{
			return EntityTag.ToString();
		}
		return Date.Value.ToString("r", CultureInfo.InvariantCulture);
	}
}

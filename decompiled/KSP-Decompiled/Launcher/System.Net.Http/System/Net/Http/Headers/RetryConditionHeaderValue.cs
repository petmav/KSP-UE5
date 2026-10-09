using System.Globalization;

namespace System.Net.Http.Headers;

public class RetryConditionHeaderValue : ICloneable
{
	public DateTimeOffset? Date { get; private set; }

	public TimeSpan? Delta { get; private set; }

	public RetryConditionHeaderValue(DateTimeOffset date)
	{
		Date = date;
	}

	public RetryConditionHeaderValue(TimeSpan delta)
	{
		if (delta.TotalSeconds > 4294967295.0)
		{
			throw new ArgumentOutOfRangeException("delta");
		}
		Delta = delta;
	}

	object ICloneable.Clone()
	{
		return MemberwiseClone();
	}

	public override bool Equals(object obj)
	{
		if (obj is RetryConditionHeaderValue { Date: var date } retryConditionHeaderValue && date == Date)
		{
			TimeSpan? delta = retryConditionHeaderValue.Delta;
			TimeSpan? delta2 = Delta;
			if (delta.HasValue != delta2.HasValue)
			{
				return false;
			}
			if (!delta.HasValue)
			{
				return true;
			}
			return delta.GetValueOrDefault() == delta2.GetValueOrDefault();
		}
		return false;
	}

	public override int GetHashCode()
	{
		return Date.GetHashCode() ^ Delta.GetHashCode();
	}

	public static RetryConditionHeaderValue Parse(string input)
	{
		if (TryParse(input, out var parsedValue))
		{
			return parsedValue;
		}
		throw new FormatException(input);
	}

	public static bool TryParse(string input, out RetryConditionHeaderValue parsedValue)
	{
		parsedValue = null;
		Lexer lexer = new Lexer(input);
		Token token = lexer.Scan();
		if ((Token.Type)token != Token.Type.Token)
		{
			return false;
		}
		TimeSpan? timeSpan = lexer.TryGetTimeSpanValue(token);
		if (timeSpan.HasValue)
		{
			if ((Token.Type)lexer.Scan() != Token.Type.End)
			{
				return false;
			}
			parsedValue = new RetryConditionHeaderValue(timeSpan.Value);
		}
		else
		{
			if (!Lexer.TryGetDateValue(input, out var value))
			{
				return false;
			}
			parsedValue = new RetryConditionHeaderValue(value);
		}
		return true;
	}

	public override string ToString()
	{
		if (!Delta.HasValue)
		{
			return Date.Value.ToString("r", CultureInfo.InvariantCulture);
		}
		return Delta.Value.TotalSeconds.ToString(CultureInfo.InvariantCulture);
	}
}

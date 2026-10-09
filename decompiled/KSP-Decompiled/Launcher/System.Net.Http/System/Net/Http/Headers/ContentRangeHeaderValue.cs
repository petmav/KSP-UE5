using System.Globalization;
using System.Text;

namespace System.Net.Http.Headers;

public class ContentRangeHeaderValue : ICloneable
{
	private string unit = "bytes";

	public long? From { get; private set; }

	public bool HasLength => Length.HasValue;

	public bool HasRange => From.HasValue;

	public long? Length { get; private set; }

	public long? To { get; private set; }

	public string Unit
	{
		get
		{
			return unit;
		}
		set
		{
			if (value == null)
			{
				throw new ArgumentNullException("Unit");
			}
			Parser.Token.Check(value);
			unit = value;
		}
	}

	private ContentRangeHeaderValue()
	{
	}

	public ContentRangeHeaderValue(long length)
	{
		if (length < 0)
		{
			throw new ArgumentOutOfRangeException("length");
		}
		Length = length;
	}

	public ContentRangeHeaderValue(long from, long to)
	{
		if (from < 0 || from > to)
		{
			throw new ArgumentOutOfRangeException("from");
		}
		From = from;
		To = to;
	}

	public ContentRangeHeaderValue(long from, long to, long length)
		: this(from, to)
	{
		if (length < 0)
		{
			throw new ArgumentOutOfRangeException("length");
		}
		if (to > length)
		{
			throw new ArgumentOutOfRangeException("to");
		}
		Length = length;
	}

	object ICloneable.Clone()
	{
		return MemberwiseClone();
	}

	public override bool Equals(object obj)
	{
		if (!(obj is ContentRangeHeaderValue { Length: var length } contentRangeHeaderValue))
		{
			return false;
		}
		if (length == Length)
		{
			long? from = contentRangeHeaderValue.From;
			long? from2 = From;
			if (from.GetValueOrDefault() == from2.GetValueOrDefault() && from.HasValue == from2.HasValue && contentRangeHeaderValue.To == To)
			{
				return string.Equals(contentRangeHeaderValue.unit, unit, StringComparison.OrdinalIgnoreCase);
			}
		}
		return false;
	}

	public override int GetHashCode()
	{
		return Unit.GetHashCode() ^ Length.GetHashCode() ^ From.GetHashCode() ^ To.GetHashCode() ^ unit.ToLowerInvariant().GetHashCode();
	}

	public static ContentRangeHeaderValue Parse(string input)
	{
		if (TryParse(input, out var parsedValue))
		{
			return parsedValue;
		}
		throw new FormatException(input);
	}

	public static bool TryParse(string input, out ContentRangeHeaderValue parsedValue)
	{
		parsedValue = null;
		Lexer lexer = new Lexer(input);
		Token token = lexer.Scan();
		if ((Token.Type)token != Token.Type.Token)
		{
			return false;
		}
		ContentRangeHeaderValue contentRangeHeaderValue = new ContentRangeHeaderValue();
		contentRangeHeaderValue.unit = lexer.GetStringValue(token);
		token = lexer.Scan();
		if ((Token.Type)token != Token.Type.Token)
		{
			return false;
		}
		if (!lexer.IsStarStringValue(token))
		{
			if (!lexer.TryGetNumericValue(token, out long value))
			{
				string stringValue = lexer.GetStringValue(token);
				if (stringValue.Length < 3)
				{
					return false;
				}
				string[] array = stringValue.Split('-');
				if (array.Length != 2)
				{
					return false;
				}
				if (!long.TryParse(array[0], NumberStyles.None, CultureInfo.InvariantCulture, out value))
				{
					return false;
				}
				contentRangeHeaderValue.From = value;
				if (!long.TryParse(array[1], NumberStyles.None, CultureInfo.InvariantCulture, out value))
				{
					return false;
				}
				contentRangeHeaderValue.To = value;
			}
			else
			{
				contentRangeHeaderValue.From = value;
				token = lexer.Scan(recognizeDash: true);
				if ((Token.Type)token != Token.Type.SeparatorDash)
				{
					return false;
				}
				token = lexer.Scan();
				if (!lexer.TryGetNumericValue(token, out value))
				{
					return false;
				}
				contentRangeHeaderValue.To = value;
			}
		}
		token = lexer.Scan();
		if ((Token.Type)token != Token.Type.SeparatorSlash)
		{
			return false;
		}
		token = lexer.Scan();
		if (!lexer.IsStarStringValue(token))
		{
			if (!lexer.TryGetNumericValue(token, out long value2))
			{
				return false;
			}
			contentRangeHeaderValue.Length = value2;
		}
		token = lexer.Scan();
		if ((Token.Type)token != Token.Type.End)
		{
			return false;
		}
		parsedValue = contentRangeHeaderValue;
		return true;
	}

	public override string ToString()
	{
		StringBuilder stringBuilder = new StringBuilder(unit);
		stringBuilder.Append(" ");
		if (!From.HasValue)
		{
			stringBuilder.Append("*");
		}
		else
		{
			stringBuilder.Append(From.Value.ToString(CultureInfo.InvariantCulture));
			stringBuilder.Append("-");
			stringBuilder.Append(To.Value.ToString(CultureInfo.InvariantCulture));
		}
		stringBuilder.Append("/");
		stringBuilder.Append((!Length.HasValue) ? "*" : Length.Value.ToString(CultureInfo.InvariantCulture));
		return stringBuilder.ToString();
	}
}

using System.Collections.Generic;
using System.Text;

namespace System.Net.Http.Headers;

public class RangeHeaderValue : ICloneable
{
	private List<RangeItemHeaderValue> ranges;

	private string unit;

	public ICollection<RangeItemHeaderValue> Ranges => ranges ?? (ranges = new List<RangeItemHeaderValue>());

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

	public RangeHeaderValue()
	{
		unit = "bytes";
	}

	public RangeHeaderValue(long? from, long? to)
		: this()
	{
		Ranges.Add(new RangeItemHeaderValue(from, to));
	}

	private RangeHeaderValue(RangeHeaderValue source)
		: this()
	{
		if (source.ranges == null)
		{
			return;
		}
		foreach (RangeItemHeaderValue range in source.ranges)
		{
			Ranges.Add(range);
		}
	}

	object ICloneable.Clone()
	{
		return new RangeHeaderValue(this);
	}

	public override bool Equals(object obj)
	{
		if (!(obj is RangeHeaderValue rangeHeaderValue))
		{
			return false;
		}
		if (string.Equals(rangeHeaderValue.Unit, Unit, StringComparison.OrdinalIgnoreCase))
		{
			return rangeHeaderValue.ranges.SequenceEqual(ranges);
		}
		return false;
	}

	public override int GetHashCode()
	{
		return Unit.ToLowerInvariant().GetHashCode() ^ HashCodeCalculator.Calculate(ranges);
	}

	public static RangeHeaderValue Parse(string input)
	{
		if (TryParse(input, out var parsedValue))
		{
			return parsedValue;
		}
		throw new FormatException(input);
	}

	public static bool TryParse(string input, out RangeHeaderValue parsedValue)
	{
		parsedValue = null;
		Lexer lexer = new Lexer(input);
		Token token = lexer.Scan();
		if ((Token.Type)token != Token.Type.Token)
		{
			return false;
		}
		RangeHeaderValue rangeHeaderValue = new RangeHeaderValue();
		rangeHeaderValue.unit = lexer.GetStringValue(token);
		token = lexer.Scan();
		if ((Token.Type)token != Token.Type.SeparatorEqual)
		{
			return false;
		}
		do
		{
			long? num = null;
			long? num2 = null;
			bool flag = false;
			token = lexer.Scan(recognizeDash: true);
			long result;
			switch (token.Kind)
			{
			case Token.Type.SeparatorDash:
				token = lexer.Scan();
				if (!lexer.TryGetNumericValue(token, out result))
				{
					return false;
				}
				num2 = result;
				break;
			case Token.Type.Token:
			{
				string stringValue = lexer.GetStringValue(token);
				string[] array = stringValue.Split(new char[1] { '-' }, StringSplitOptions.RemoveEmptyEntries);
				if (!Parser.Long.TryParse(array[0], out result))
				{
					return false;
				}
				switch (array.Length)
				{
				case 1:
					token = lexer.Scan(recognizeDash: true);
					num = result;
					switch (token.Kind)
					{
					case Token.Type.SeparatorDash:
						token = lexer.Scan();
						if ((Token.Type)token != Token.Type.Token)
						{
							flag = true;
							break;
						}
						if (!lexer.TryGetNumericValue(token, out result))
						{
							return false;
						}
						num2 = result;
						if (!(num2 < num))
						{
							break;
						}
						return false;
					case Token.Type.End:
						if (stringValue.Length > 0 && stringValue[stringValue.Length - 1] != '-')
						{
							return false;
						}
						flag = true;
						break;
					case Token.Type.SeparatorComma:
						flag = true;
						break;
					default:
						return false;
					}
					break;
				case 2:
					num = result;
					if (!Parser.Long.TryParse(array[1], out result))
					{
						return false;
					}
					num2 = result;
					if (num2 < num)
					{
						return false;
					}
					break;
				default:
					return false;
				}
				break;
			}
			default:
				return false;
			}
			rangeHeaderValue.Ranges.Add(new RangeItemHeaderValue(num, num2));
			if (!flag)
			{
				token = lexer.Scan();
			}
		}
		while ((Token.Type)token == Token.Type.SeparatorComma);
		if ((Token.Type)token != Token.Type.End)
		{
			return false;
		}
		parsedValue = rangeHeaderValue;
		return true;
	}

	public override string ToString()
	{
		StringBuilder stringBuilder = new StringBuilder(unit);
		stringBuilder.Append("=");
		for (int i = 0; i < Ranges.Count; i++)
		{
			if (i > 0)
			{
				stringBuilder.Append(", ");
			}
			stringBuilder.Append(ranges[i]);
		}
		return stringBuilder.ToString();
	}
}

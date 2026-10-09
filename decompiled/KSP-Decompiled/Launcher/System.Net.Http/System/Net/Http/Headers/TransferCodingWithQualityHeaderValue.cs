using System.Collections.Generic;

namespace System.Net.Http.Headers;

public sealed class TransferCodingWithQualityHeaderValue : TransferCodingHeaderValue
{
	public double? Quality
	{
		get
		{
			return QualityValue.GetValue(parameters);
		}
		set
		{
			QualityValue.SetValue(ref parameters, value);
		}
	}

	public TransferCodingWithQualityHeaderValue(string value)
		: base(value)
	{
	}

	public TransferCodingWithQualityHeaderValue(string value, double quality)
		: this(value)
	{
		Quality = quality;
	}

	private TransferCodingWithQualityHeaderValue()
	{
	}

	public new static TransferCodingWithQualityHeaderValue Parse(string input)
	{
		if (TryParse(input, out var parsedValue))
		{
			return parsedValue;
		}
		throw new FormatException();
	}

	public static bool TryParse(string input, out TransferCodingWithQualityHeaderValue parsedValue)
	{
		if (TryParseElement(new Lexer(input), out parsedValue, out var t) && (Token.Type)t == Token.Type.End)
		{
			return true;
		}
		parsedValue = null;
		return false;
	}

	internal static bool TryParse(string input, int minimalCount, out List<TransferCodingWithQualityHeaderValue> result)
	{
		return CollectionParser.TryParse(input, minimalCount, (ElementTryParser<TransferCodingWithQualityHeaderValue>)TryParseElement, out result);
	}

	private static bool TryParseElement(Lexer lexer, out TransferCodingWithQualityHeaderValue parsedValue, out Token t)
	{
		parsedValue = null;
		t = lexer.Scan();
		if ((Token.Type)t != Token.Type.Token)
		{
			return false;
		}
		TransferCodingWithQualityHeaderValue transferCodingWithQualityHeaderValue = new TransferCodingWithQualityHeaderValue();
		transferCodingWithQualityHeaderValue.value = lexer.GetStringValue(t);
		t = lexer.Scan();
		if ((Token.Type)t == Token.Type.SeparatorSemicolon && (!NameValueHeaderValue.TryParseParameters(lexer, out transferCodingWithQualityHeaderValue.parameters, out t) || (Token.Type)t != Token.Type.End))
		{
			return false;
		}
		parsedValue = transferCodingWithQualityHeaderValue;
		return true;
	}
}

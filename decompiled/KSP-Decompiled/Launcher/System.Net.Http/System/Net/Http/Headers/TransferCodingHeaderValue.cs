using System.Collections.Generic;

namespace System.Net.Http.Headers;

public class TransferCodingHeaderValue : ICloneable
{
	internal string value;

	internal List<NameValueHeaderValue> parameters;

	public ICollection<NameValueHeaderValue> Parameters => parameters ?? (parameters = new List<NameValueHeaderValue>());

	public string Value => value;

	public TransferCodingHeaderValue(string value)
	{
		Parser.Token.Check(value);
		this.value = value;
	}

	protected TransferCodingHeaderValue(TransferCodingHeaderValue source)
	{
		value = source.value;
		if (source.parameters == null)
		{
			return;
		}
		foreach (NameValueHeaderValue parameter in source.parameters)
		{
			Parameters.Add(new NameValueHeaderValue(parameter));
		}
	}

	internal TransferCodingHeaderValue()
	{
	}

	object ICloneable.Clone()
	{
		return new TransferCodingHeaderValue(this);
	}

	public override bool Equals(object obj)
	{
		if (obj is TransferCodingHeaderValue transferCodingHeaderValue && string.Equals(value, transferCodingHeaderValue.value, StringComparison.OrdinalIgnoreCase))
		{
			return parameters.SequenceEqual(transferCodingHeaderValue.parameters);
		}
		return false;
	}

	public override int GetHashCode()
	{
		int num = value.ToLowerInvariant().GetHashCode();
		if (parameters != null)
		{
			num ^= HashCodeCalculator.Calculate(parameters);
		}
		return num;
	}

	public static TransferCodingHeaderValue Parse(string input)
	{
		if (TryParse(input, out var parsedValue))
		{
			return parsedValue;
		}
		throw new FormatException(input);
	}

	public override string ToString()
	{
		return value + CollectionExtensions.ToString(parameters);
	}

	public static bool TryParse(string input, out TransferCodingHeaderValue parsedValue)
	{
		if (TryParseElement(new Lexer(input), out parsedValue, out var t) && (Token.Type)t == Token.Type.End)
		{
			return true;
		}
		parsedValue = null;
		return false;
	}

	internal static bool TryParse(string input, int minimalCount, out List<TransferCodingHeaderValue> result)
	{
		return CollectionParser.TryParse(input, minimalCount, (ElementTryParser<TransferCodingHeaderValue>)TryParseElement, out result);
	}

	private static bool TryParseElement(Lexer lexer, out TransferCodingHeaderValue parsedValue, out Token t)
	{
		parsedValue = null;
		t = lexer.Scan();
		if ((Token.Type)t != Token.Type.Token)
		{
			return false;
		}
		TransferCodingHeaderValue transferCodingHeaderValue = new TransferCodingHeaderValue();
		transferCodingHeaderValue.value = lexer.GetStringValue(t);
		t = lexer.Scan();
		if ((Token.Type)t == Token.Type.SeparatorSemicolon && (!NameValueHeaderValue.TryParseParameters(lexer, out transferCodingHeaderValue.parameters, out t) || (Token.Type)t != Token.Type.End))
		{
			return false;
		}
		parsedValue = transferCodingHeaderValue;
		return true;
	}
}

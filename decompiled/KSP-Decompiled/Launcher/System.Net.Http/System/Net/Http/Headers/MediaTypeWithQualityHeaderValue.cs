using System.Collections.Generic;

namespace System.Net.Http.Headers;

public sealed class MediaTypeWithQualityHeaderValue : MediaTypeHeaderValue
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

	public MediaTypeWithQualityHeaderValue(string mediaType)
		: base(mediaType)
	{
	}

	public MediaTypeWithQualityHeaderValue(string mediaType, double quality)
		: this(mediaType)
	{
		Quality = quality;
	}

	private MediaTypeWithQualityHeaderValue()
	{
	}

	public new static MediaTypeWithQualityHeaderValue Parse(string input)
	{
		if (TryParse(input, out var parsedValue))
		{
			return parsedValue;
		}
		throw new FormatException();
	}

	public static bool TryParse(string input, out MediaTypeWithQualityHeaderValue parsedValue)
	{
		if (TryParseElement(new Lexer(input), out parsedValue, out var t) && (Token.Type)t == Token.Type.End)
		{
			return true;
		}
		parsedValue = null;
		return false;
	}

	private static bool TryParseElement(Lexer lexer, out MediaTypeWithQualityHeaderValue parsedValue, out Token t)
	{
		parsedValue = null;
		List<NameValueHeaderValue> result = null;
		string media;
		Token? token = MediaTypeHeaderValue.TryParseMediaType(lexer, out media);
		if (!token.HasValue)
		{
			t = Token.Empty;
			return false;
		}
		t = token.Value;
		if ((Token.Type)t == Token.Type.SeparatorSemicolon && !NameValueHeaderValue.TryParseParameters(lexer, out result, out t))
		{
			return false;
		}
		parsedValue = new MediaTypeWithQualityHeaderValue();
		parsedValue.media_type = media;
		parsedValue.parameters = result;
		return true;
	}

	internal static bool TryParse(string input, int minimalCount, out List<MediaTypeWithQualityHeaderValue> result)
	{
		return CollectionParser.TryParse(input, minimalCount, (ElementTryParser<MediaTypeWithQualityHeaderValue>)TryParseElement, out result);
	}
}

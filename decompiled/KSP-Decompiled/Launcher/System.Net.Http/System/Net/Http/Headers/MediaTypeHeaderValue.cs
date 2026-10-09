using System.Collections.Generic;

namespace System.Net.Http.Headers;

public class MediaTypeHeaderValue : ICloneable
{
	internal List<NameValueHeaderValue> parameters;

	internal string media_type;

	public string CharSet
	{
		get
		{
			if (parameters == null)
			{
				return null;
			}
			return parameters.Find((NameValueHeaderValue l) => string.Equals(l.Name, "charset", StringComparison.OrdinalIgnoreCase))?.Value;
		}
		set
		{
			if (parameters == null)
			{
				parameters = new List<NameValueHeaderValue>();
			}
			parameters.SetValue("charset", value);
		}
	}

	public string MediaType
	{
		get
		{
			return media_type;
		}
		set
		{
			if (value == null)
			{
				throw new ArgumentNullException("MediaType");
			}
			string media;
			Token? token = TryParseMediaType(new Lexer(value), out media);
			if (!token.HasValue || token.Value.Kind != Token.Type.End)
			{
				throw new FormatException();
			}
			media_type = media;
		}
	}

	public ICollection<NameValueHeaderValue> Parameters => parameters ?? (parameters = new List<NameValueHeaderValue>());

	public MediaTypeHeaderValue(string mediaType)
	{
		MediaType = mediaType;
	}

	protected MediaTypeHeaderValue(MediaTypeHeaderValue source)
	{
		if (source == null)
		{
			throw new ArgumentNullException("source");
		}
		media_type = source.media_type;
		if (source.parameters == null)
		{
			return;
		}
		foreach (NameValueHeaderValue parameter in source.parameters)
		{
			Parameters.Add(new NameValueHeaderValue(parameter));
		}
	}

	internal MediaTypeHeaderValue()
	{
	}

	object ICloneable.Clone()
	{
		return new MediaTypeHeaderValue(this);
	}

	public override bool Equals(object obj)
	{
		if (!(obj is MediaTypeHeaderValue mediaTypeHeaderValue))
		{
			return false;
		}
		if (string.Equals(mediaTypeHeaderValue.media_type, media_type, StringComparison.OrdinalIgnoreCase))
		{
			return mediaTypeHeaderValue.parameters.SequenceEqual(parameters);
		}
		return false;
	}

	public override int GetHashCode()
	{
		return media_type.ToLowerInvariant().GetHashCode() ^ HashCodeCalculator.Calculate(parameters);
	}

	public static MediaTypeHeaderValue Parse(string input)
	{
		if (TryParse(input, out var parsedValue))
		{
			return parsedValue;
		}
		throw new FormatException(input);
	}

	public override string ToString()
	{
		if (parameters == null)
		{
			return media_type;
		}
		return media_type + CollectionExtensions.ToString(parameters);
	}

	public static bool TryParse(string input, out MediaTypeHeaderValue parsedValue)
	{
		parsedValue = null;
		Lexer lexer = new Lexer(input);
		List<NameValueHeaderValue> result = null;
		string media;
		Token? token = TryParseMediaType(lexer, out media);
		if (!token.HasValue)
		{
			return false;
		}
		switch (token.Value.Kind)
		{
		case Token.Type.SeparatorSemicolon:
		{
			if (!NameValueHeaderValue.TryParseParameters(lexer, out result, out var t) || (Token.Type)t != Token.Type.End)
			{
				return false;
			}
			break;
		}
		default:
			return false;
		case Token.Type.End:
			break;
		}
		parsedValue = new MediaTypeHeaderValue
		{
			media_type = media,
			parameters = result
		};
		return true;
	}

	internal static Token? TryParseMediaType(Lexer lexer, out string media)
	{
		media = null;
		Token token = lexer.Scan();
		if ((Token.Type)token != Token.Type.Token)
		{
			return null;
		}
		if ((Token.Type)lexer.Scan() != Token.Type.SeparatorSlash)
		{
			return null;
		}
		Token token2 = lexer.Scan();
		if ((Token.Type)token2 != Token.Type.Token)
		{
			return null;
		}
		media = lexer.GetStringValue(token) + "/" + lexer.GetStringValue(token2);
		return lexer.Scan();
	}
}

using System.Collections.Generic;
using System.Globalization;
using System.Text;

namespace System.Net.Http.Headers;

public class ContentDispositionHeaderValue : ICloneable
{
	private string dispositionType;

	private List<NameValueHeaderValue> parameters;

	public DateTimeOffset? CreationDate
	{
		get
		{
			return GetDateValue("creation-date");
		}
		set
		{
			SetDateValue("creation-date", value);
		}
	}

	public string DispositionType
	{
		get
		{
			return dispositionType;
		}
		set
		{
			Parser.Token.Check(value);
			dispositionType = value;
		}
	}

	public string FileName
	{
		get
		{
			string text = FindParameter("filename");
			if (text == null)
			{
				return null;
			}
			return DecodeValue(text, extendedNotation: false);
		}
		set
		{
			if (value != null)
			{
				value = EncodeBase64Value(value);
			}
			SetValue("filename", value);
		}
	}

	public string FileNameStar
	{
		get
		{
			string text = FindParameter("filename*");
			if (text == null)
			{
				return null;
			}
			return DecodeValue(text, extendedNotation: true);
		}
		set
		{
			if (value != null)
			{
				value = EncodeRFC5987(value);
			}
			SetValue("filename*", value);
		}
	}

	public DateTimeOffset? ModificationDate
	{
		get
		{
			return GetDateValue("modification-date");
		}
		set
		{
			SetDateValue("modification-date", value);
		}
	}

	public string Name
	{
		get
		{
			string text = FindParameter("name");
			if (text == null)
			{
				return null;
			}
			return DecodeValue(text, extendedNotation: false);
		}
		set
		{
			if (value != null)
			{
				value = EncodeBase64Value(value);
			}
			SetValue("name", value);
		}
	}

	public ICollection<NameValueHeaderValue> Parameters => parameters ?? (parameters = new List<NameValueHeaderValue>());

	public DateTimeOffset? ReadDate
	{
		get
		{
			return GetDateValue("read-date");
		}
		set
		{
			SetDateValue("read-date", value);
		}
	}

	public long? Size
	{
		get
		{
			if (Parser.Long.TryParse(FindParameter("size"), out var result))
			{
				return result;
			}
			return null;
		}
		set
		{
			if (!value.HasValue)
			{
				SetValue("size", null);
				return;
			}
			if (value < 0)
			{
				throw new ArgumentOutOfRangeException("value");
			}
			SetValue("size", value.Value.ToString(CultureInfo.InvariantCulture));
		}
	}

	private ContentDispositionHeaderValue()
	{
	}

	public ContentDispositionHeaderValue(string dispositionType)
	{
		DispositionType = dispositionType;
	}

	protected ContentDispositionHeaderValue(ContentDispositionHeaderValue source)
	{
		if (source == null)
		{
			throw new ArgumentNullException("source");
		}
		dispositionType = source.dispositionType;
		if (source.parameters == null)
		{
			return;
		}
		foreach (NameValueHeaderValue parameter in source.parameters)
		{
			Parameters.Add(new NameValueHeaderValue(parameter));
		}
	}

	object ICloneable.Clone()
	{
		return new ContentDispositionHeaderValue(this);
	}

	public override bool Equals(object obj)
	{
		if (obj is ContentDispositionHeaderValue contentDispositionHeaderValue && string.Equals(contentDispositionHeaderValue.dispositionType, dispositionType, StringComparison.OrdinalIgnoreCase))
		{
			return contentDispositionHeaderValue.parameters.SequenceEqual(parameters);
		}
		return false;
	}

	private string FindParameter(string name)
	{
		if (parameters == null)
		{
			return null;
		}
		foreach (NameValueHeaderValue parameter in parameters)
		{
			if (string.Equals(parameter.Name, name, StringComparison.OrdinalIgnoreCase))
			{
				return parameter.Value;
			}
		}
		return null;
	}

	private DateTimeOffset? GetDateValue(string name)
	{
		string text = FindParameter(name);
		if (text == null || text == null)
		{
			return null;
		}
		if (text.Length < 3)
		{
			return null;
		}
		if (text[0] == '"')
		{
			text = text.Substring(1, text.Length - 2);
		}
		if (Lexer.TryGetDateValue(text, out var value))
		{
			return value;
		}
		return null;
	}

	private static string EncodeBase64Value(string value)
	{
		bool flag = value.Length > 1 && value[0] == '"' && value[value.Length - 1] == '"';
		if (flag)
		{
			value = value.Substring(1, value.Length - 2);
		}
		for (int i = 0; i < value.Length; i++)
		{
			if (value[i] > '\u007f')
			{
				Encoding uTF = Encoding.UTF8;
				return $"\"=?{uTF.WebName}?B?{Convert.ToBase64String(uTF.GetBytes(value))}?=\"";
			}
		}
		if (flag || !Lexer.IsValidToken(value))
		{
			return "\"" + value + "\"";
		}
		return value;
	}

	private static string EncodeRFC5987(string value)
	{
		Encoding uTF = Encoding.UTF8;
		StringBuilder stringBuilder = new StringBuilder(value.Length + 11);
		stringBuilder.Append(uTF.WebName);
		stringBuilder.Append('\'');
		stringBuilder.Append('\'');
		foreach (char c in value)
		{
			if (c > '\u007f')
			{
				byte[] bytes = uTF.GetBytes(new char[1] { c });
				foreach (byte b in bytes)
				{
					stringBuilder.Append('%');
					stringBuilder.Append(b.ToString("X2"));
				}
			}
			else if (!Lexer.IsValidCharacter(c) || c == '*' || c == '?' || c == '%')
			{
				stringBuilder.Append(Uri.HexEscape(c));
			}
			else
			{
				stringBuilder.Append(c);
			}
		}
		return stringBuilder.ToString();
	}

	private static string DecodeValue(string value, bool extendedNotation)
	{
		if (value.Length < 2)
		{
			return value;
		}
		string[] array;
		Encoding encoding;
		if (value[0] == '"')
		{
			array = value.Split('?');
			if (array.Length != 5 || array[0] != "\"=" || array[4] != "=\"" || (array[2] != "B" && array[2] != "b"))
			{
				return value;
			}
			try
			{
				encoding = Encoding.GetEncoding(array[1]);
				return encoding.GetString(Convert.FromBase64String(array[3]));
			}
			catch
			{
				return value;
			}
		}
		if (!extendedNotation)
		{
			return value;
		}
		array = value.Split('\'');
		if (array.Length != 3)
		{
			return null;
		}
		try
		{
			encoding = Encoding.GetEncoding(array[0]);
		}
		catch
		{
			return null;
		}
		value = array[2];
		if (value.IndexOf('%') < 0)
		{
			return value;
		}
		StringBuilder stringBuilder = new StringBuilder();
		byte[] array2 = null;
		int num = 0;
		int index = 0;
		while (index < value.Length)
		{
			char c = value[index];
			if (c == '%')
			{
				char c2 = c;
				c = Uri.HexUnescape(value, ref index);
				if (c != c2)
				{
					if (array2 == null)
					{
						array2 = new byte[value.Length - index + 1];
					}
					array2[num++] = (byte)c;
					continue;
				}
			}
			else
			{
				index++;
			}
			if (num != 0)
			{
				stringBuilder.Append(encoding.GetChars(array2, 0, num));
				num = 0;
			}
			stringBuilder.Append(c);
		}
		if (num != 0)
		{
			stringBuilder.Append(encoding.GetChars(array2, 0, num));
		}
		return stringBuilder.ToString();
	}

	public override int GetHashCode()
	{
		return dispositionType.ToLowerInvariant().GetHashCode() ^ HashCodeCalculator.Calculate(parameters);
	}

	public static ContentDispositionHeaderValue Parse(string input)
	{
		if (TryParse(input, out var parsedValue))
		{
			return parsedValue;
		}
		throw new FormatException(input);
	}

	private void SetDateValue(string key, DateTimeOffset? value)
	{
		SetValue(key, (!value.HasValue) ? null : ("\"" + value.Value.ToString("r", CultureInfo.InvariantCulture) + "\""));
	}

	private void SetValue(string key, string value)
	{
		if (parameters == null)
		{
			parameters = new List<NameValueHeaderValue>();
		}
		parameters.SetValue(key, value);
	}

	public override string ToString()
	{
		return dispositionType + CollectionExtensions.ToString(parameters);
	}

	public static bool TryParse(string input, out ContentDispositionHeaderValue parsedValue)
	{
		parsedValue = null;
		Lexer lexer = new Lexer(input);
		Token token = lexer.Scan();
		if (token.Kind != Token.Type.Token)
		{
			return false;
		}
		List<NameValueHeaderValue> result = null;
		string stringValue = lexer.GetStringValue(token);
		token = lexer.Scan();
		switch (token.Kind)
		{
		case Token.Type.SeparatorSemicolon:
			if (!NameValueHeaderValue.TryParseParameters(lexer, out result, out token) || (Token.Type)token != Token.Type.End)
			{
				return false;
			}
			break;
		default:
			return false;
		case Token.Type.End:
			break;
		}
		parsedValue = new ContentDispositionHeaderValue
		{
			dispositionType = stringValue,
			parameters = result
		};
		return true;
	}
}

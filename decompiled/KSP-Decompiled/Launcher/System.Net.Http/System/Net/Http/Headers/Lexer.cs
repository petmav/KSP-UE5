using System.Globalization;

namespace System.Net.Http.Headers;

internal class Lexer
{
	private static readonly bool[] token_chars = new bool[127]
	{
		false, false, false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false, false, false,
		false, false, false, true, false, true, true, true, true, true,
		false, false, true, true, false, true, true, false, true, true,
		true, true, true, true, true, true, true, true, false, false,
		false, false, false, false, false, true, true, true, true, true,
		true, true, true, true, true, true, true, true, true, true,
		true, true, true, true, true, true, true, true, true, true,
		true, false, false, false, true, true, true, true, true, true,
		true, true, true, true, true, true, true, true, true, true,
		true, true, true, true, true, true, true, true, true, true,
		true, true, true, false, true, false, true
	};

	private static readonly int last_token_char = token_chars.Length;

	private static readonly string[] dt_formats = new string[5] { "r", "dddd, dd'-'MMM'-'yy HH:mm:ss 'GMT'", "ddd MMM d HH:mm:ss yyyy", "d MMM yy H:m:s", "ddd, d MMM yyyy H:m:s zzz" };

	private readonly string s;

	private int pos;

	public int Position
	{
		get
		{
			return pos;
		}
		set
		{
			pos = value;
		}
	}

	public Lexer(string stream)
	{
		s = stream;
	}

	public string GetStringValue(Token token)
	{
		return s.Substring(token.StartPosition, token.EndPosition - token.StartPosition);
	}

	public string GetStringValue(Token start, Token end)
	{
		return s.Substring(start.StartPosition, end.EndPosition - start.StartPosition);
	}

	public string GetQuotedStringValue(Token start)
	{
		return s.Substring(start.StartPosition + 1, start.EndPosition - start.StartPosition - 2);
	}

	public string GetRemainingStringValue(int position)
	{
		if (position <= s.Length)
		{
			return s.Substring(position);
		}
		return null;
	}

	public bool IsStarStringValue(Token token)
	{
		if (token.EndPosition - token.StartPosition == 1)
		{
			return s[token.StartPosition] == '*';
		}
		return false;
	}

	public bool TryGetNumericValue(Token token, out int value)
	{
		return int.TryParse(GetStringValue(token), NumberStyles.None, CultureInfo.InvariantCulture, out value);
	}

	public bool TryGetNumericValue(Token token, out long value)
	{
		return long.TryParse(GetStringValue(token), NumberStyles.None, CultureInfo.InvariantCulture, out value);
	}

	public TimeSpan? TryGetTimeSpanValue(Token token)
	{
		if (TryGetNumericValue(token, out int value))
		{
			return TimeSpan.FromSeconds(value);
		}
		return null;
	}

	public bool TryGetDateValue(Token token, out DateTimeOffset value)
	{
		return TryGetDateValue(((Token.Type)token == Token.Type.QuotedString) ? s.Substring(token.StartPosition + 1, token.EndPosition - token.StartPosition - 2) : GetStringValue(token), out value);
	}

	public static bool TryGetDateValue(string text, out DateTimeOffset value)
	{
		return DateTimeOffset.TryParseExact(text, dt_formats, DateTimeFormatInfo.InvariantInfo, DateTimeStyles.AllowWhiteSpaces | DateTimeStyles.AssumeUniversal, out value);
	}

	public bool TryGetDoubleValue(Token token, out double value)
	{
		return double.TryParse(GetStringValue(token), NumberStyles.AllowDecimalPoint, CultureInfo.InvariantCulture, out value);
	}

	public static bool IsValidToken(string input)
	{
		int i;
		for (i = 0; i < input.Length; i++)
		{
			if (!IsValidCharacter(input[i]))
			{
				return false;
			}
		}
		return i > 0;
	}

	public static bool IsValidCharacter(char input)
	{
		if (input < last_token_char)
		{
			return token_chars[(uint)input];
		}
		return false;
	}

	public void EatChar()
	{
		pos++;
	}

	public int PeekChar()
	{
		if (pos >= s.Length)
		{
			return -1;
		}
		return s[pos];
	}

	public bool ScanCommentOptional(out string value)
	{
		if (ScanCommentOptional(out value, out var readToken))
		{
			return true;
		}
		return (Token.Type)readToken == Token.Type.End;
	}

	public bool ScanCommentOptional(out string value, out Token readToken)
	{
		readToken = Scan();
		if ((Token.Type)readToken != Token.Type.OpenParens)
		{
			value = null;
			return false;
		}
		int num = 1;
		while (pos < s.Length)
		{
			switch (s[pos])
			{
			case '(':
				num++;
				pos++;
				continue;
			case ')':
			{
				pos++;
				if (--num > 0)
				{
					continue;
				}
				int startPosition = readToken.StartPosition;
				value = s.Substring(startPosition, pos - startPosition);
				return true;
			}
			case ' ':
			case '!':
			case '"':
			case '#':
			case '$':
			case '%':
			case '&':
			case '\'':
			case '*':
			case '+':
			case ',':
			case '-':
			case '.':
			case '/':
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case ':':
			case ';':
			case '<':
			case '=':
			case '>':
			case '?':
			case '@':
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
			case 'G':
			case 'H':
			case 'I':
			case 'J':
			case 'K':
			case 'L':
			case 'M':
			case 'N':
			case 'O':
			case 'P':
			case 'Q':
			case 'R':
			case 'S':
			case 'T':
			case 'U':
			case 'V':
			case 'W':
			case 'X':
			case 'Y':
			case 'Z':
			case '[':
			case '\\':
			case ']':
			case '^':
			case '_':
			case '`':
			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f':
			case 'g':
			case 'h':
			case 'i':
			case 'j':
			case 'k':
			case 'l':
			case 'm':
			case 'n':
			case 'o':
			case 'p':
			case 'q':
			case 'r':
			case 's':
			case 't':
			case 'u':
			case 'v':
			case 'w':
			case 'x':
			case 'y':
			case 'z':
			case '{':
			case '|':
			case '}':
			case '~':
				pos++;
				continue;
			}
			break;
		}
		value = null;
		return false;
	}

	public Token Scan(bool recognizeDash = false)
	{
		int startPosition = pos;
		if (s == null)
		{
			return new Token(Token.Type.Error, 0, 0);
		}
		Token.Type type;
		if (pos >= s.Length)
		{
			type = Token.Type.End;
		}
		else
		{
			type = Token.Type.Error;
			while (true)
			{
				char c = s[pos++];
				switch (c)
				{
				case '\t':
				case ' ':
					goto IL_00a7;
				case '=':
					type = Token.Type.SeparatorEqual;
					break;
				case ';':
					type = Token.Type.SeparatorSemicolon;
					break;
				case '/':
					type = Token.Type.SeparatorSlash;
					break;
				case '-':
					if (recognizeDash)
					{
						type = Token.Type.SeparatorDash;
						break;
					}
					goto default;
				case ',':
					type = Token.Type.SeparatorComma;
					break;
				case '"':
					startPosition = pos - 1;
					while (pos < s.Length)
					{
						switch (s[pos++])
						{
						case '\\':
							if (pos + 1 < s.Length)
							{
								pos++;
								continue;
							}
							break;
						case '"':
							type = Token.Type.QuotedString;
							break;
						default:
							continue;
						}
						break;
					}
					break;
				case '(':
					startPosition = pos - 1;
					type = Token.Type.OpenParens;
					break;
				default:
					if (c >= last_token_char || !token_chars[(uint)c])
					{
						break;
					}
					startPosition = pos - 1;
					type = Token.Type.Token;
					while (pos < s.Length)
					{
						c = s[pos];
						if (c >= last_token_char || !token_chars[(uint)c])
						{
							break;
						}
						pos++;
					}
					break;
				}
				break;
				IL_00a7:
				if (pos == s.Length)
				{
					type = Token.Type.End;
					break;
				}
			}
		}
		return new Token(type, startPosition, pos);
	}
}

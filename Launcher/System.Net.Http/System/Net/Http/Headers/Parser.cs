using System.Globalization;
using System.Net.Mail;

namespace System.Net.Http.Headers;

internal static class Parser
{
	public static class Token
	{
		public static bool TryParse(string input, out string result)
		{
			if (input != null && Lexer.IsValidToken(input))
			{
				result = input;
				return true;
			}
			result = null;
			return false;
		}

		public static void Check(string s)
		{
			if (s == null)
			{
				throw new ArgumentNullException();
			}
			if (!Lexer.IsValidToken(s))
			{
				if (s.Length == 0)
				{
					throw new ArgumentException();
				}
				throw new FormatException(s);
			}
		}

		public static bool TryCheck(string s)
		{
			if (s == null)
			{
				return false;
			}
			return Lexer.IsValidToken(s);
		}

		public static void CheckQuotedString(string s)
		{
			if (s == null)
			{
				throw new ArgumentNullException();
			}
			Lexer lexer = new Lexer(s);
			if ((System.Net.Http.Headers.Token.Type)lexer.Scan() == System.Net.Http.Headers.Token.Type.QuotedString && (System.Net.Http.Headers.Token.Type)lexer.Scan() == System.Net.Http.Headers.Token.Type.End)
			{
				return;
			}
			if (s.Length == 0)
			{
				throw new ArgumentException();
			}
			throw new FormatException(s);
		}

		public static void CheckComment(string s)
		{
			if (s == null)
			{
				throw new ArgumentNullException();
			}
			if (!new Lexer(s).ScanCommentOptional(out var _))
			{
				if (s.Length == 0)
				{
					throw new ArgumentException();
				}
				throw new FormatException(s);
			}
		}
	}

	public static class DateTime
	{
		public new static readonly Func<object, string> ToString = (object l) => ((DateTimeOffset)l).ToString("r", CultureInfo.InvariantCulture);

		public static bool TryParse(string input, out DateTimeOffset result)
		{
			return Lexer.TryGetDateValue(input, out result);
		}
	}

	public static class EmailAddress
	{
		public static bool TryParse(string input, out string result)
		{
			try
			{
				new MailAddress(input);
				result = input;
				return true;
			}
			catch
			{
				result = null;
				return false;
			}
		}
	}

	public static class Host
	{
		public static bool TryParse(string input, out string result)
		{
			result = input;
			System.Uri result2;
			return System.Uri.TryCreate("http://u@" + input + "/", UriKind.Absolute, out result2);
		}
	}

	public static class Int
	{
		public static bool TryParse(string input, out int result)
		{
			return int.TryParse(input, NumberStyles.None, CultureInfo.InvariantCulture, out result);
		}
	}

	public static class Long
	{
		public static bool TryParse(string input, out long result)
		{
			return long.TryParse(input, NumberStyles.None, CultureInfo.InvariantCulture, out result);
		}
	}

	public static class MD5
	{
		public new static readonly Func<object, string> ToString = (object l) => Convert.ToBase64String((byte[])l);

		public static bool TryParse(string input, out byte[] result)
		{
			try
			{
				result = Convert.FromBase64String(input);
				return true;
			}
			catch
			{
				result = null;
				return false;
			}
		}
	}

	public static class TimeSpanSeconds
	{
		public static bool TryParse(string input, out TimeSpan result)
		{
			if (Int.TryParse(input, out var result2))
			{
				result = TimeSpan.FromSeconds(result2);
				return true;
			}
			result = TimeSpan.Zero;
			return false;
		}
	}

	public static class Uri
	{
		public static bool TryParse(string input, out System.Uri result)
		{
			return System.Uri.TryCreate(input, UriKind.RelativeOrAbsolute, out result);
		}

		public static void Check(string s)
		{
			if (s == null)
			{
				throw new ArgumentNullException();
			}
			if (!TryParse(s, out var _))
			{
				if (s.Length == 0)
				{
					throw new ArgumentException();
				}
				throw new FormatException(s);
			}
		}
	}
}

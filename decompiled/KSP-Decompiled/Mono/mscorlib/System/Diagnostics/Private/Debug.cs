#define DEBUG
using Internal.Runtime.Augments;

namespace System.Diagnostics.Private;

internal static class Debug
{
	private sealed class DebugAssertException : Exception
	{
		internal DebugAssertException(string message, string detailMessage, string stackTrace)
			: base(message + Environment.NewLine + detailMessage + Environment.NewLine + stackTrace)
		{
		}
	}

	private static readonly object s_lock = new object();

	[ThreadStatic]
	private static int s_indentLevel;

	private static int s_indentSize = 4;

	private static bool s_needIndent;

	private static string s_indentString;

	internal static Action<string, string, string> s_ShowAssertDialog = ShowAssertDialog;

	internal static Action<string> s_WriteCore = WriteCore;

	public static bool AutoFlush
	{
		get
		{
			return true;
		}
		set
		{
		}
	}

	public static int IndentLevel
	{
		get
		{
			return s_indentLevel;
		}
		set
		{
			s_indentLevel = ((value >= 0) ? value : 0);
		}
	}

	public static int IndentSize
	{
		get
		{
			return s_indentSize;
		}
		set
		{
			s_indentSize = ((value >= 0) ? value : 0);
		}
	}

	[Conditional("DEBUG")]
	public static void Close()
	{
	}

	[Conditional("DEBUG")]
	public static void Flush()
	{
	}

	[Conditional("DEBUG")]
	public static void Indent()
	{
		IndentLevel++;
	}

	[Conditional("DEBUG")]
	public static void Unindent()
	{
		IndentLevel--;
	}

	[Conditional("DEBUG")]
	public static void Print(string message)
	{
		Write(message);
	}

	[Conditional("DEBUG")]
	public static void Print(string format, params object[] args)
	{
		Write(string.Format(null, format, args));
	}

	[Conditional("DEBUG")]
	public static void Assert(bool condition)
	{
		Assert(condition, string.Empty, string.Empty);
	}

	[Conditional("DEBUG")]
	public static void Assert(bool condition, string message)
	{
		Assert(condition, message, string.Empty);
	}

	[Conditional("DEBUG")]
	public static void Assert(bool condition, string message, string detailMessage)
	{
		if (!condition)
		{
			string text;
			try
			{
				text = EnvironmentAugments.StackTrace;
			}
			catch
			{
				text = "";
			}
			WriteLine(FormatAssert(text, message, detailMessage));
			s_ShowAssertDialog(text, message, detailMessage);
		}
	}

	[Conditional("DEBUG")]
	public static void Fail(string message)
	{
		Assert(condition: false, message, string.Empty);
	}

	[Conditional("DEBUG")]
	public static void Fail(string message, string detailMessage)
	{
		Assert(condition: false, message, detailMessage);
	}

	private static string FormatAssert(string stackTrace, string message, string detailMessage)
	{
		string text = GetIndentString() + Environment.NewLine;
		return "---- DEBUG ASSERTION FAILED ----" + text + "---- Assert Short Message ----" + text + message + text + "---- Assert Long Message ----" + text + detailMessage + text + stackTrace;
	}

	[Conditional("DEBUG")]
	public static void Assert(bool condition, string message, string detailMessageFormat, params object[] args)
	{
		Assert(condition, message, string.Format(detailMessageFormat, args));
	}

	[Conditional("DEBUG")]
	public static void WriteLine(string message)
	{
		Write(message + Environment.NewLine);
	}

	[Conditional("DEBUG")]
	public static void Write(string message)
	{
		lock (s_lock)
		{
			if (message == null)
			{
				s_WriteCore(string.Empty);
				return;
			}
			if (s_needIndent)
			{
				message = GetIndentString() + message;
				s_needIndent = false;
			}
			s_WriteCore(message);
			if (message.EndsWith(Environment.NewLine))
			{
				s_needIndent = true;
			}
		}
	}

	[Conditional("DEBUG")]
	public static void WriteLine(object value)
	{
		WriteLine(value?.ToString());
	}

	[Conditional("DEBUG")]
	public static void WriteLine(object value, string category)
	{
		WriteLine(value?.ToString(), category);
	}

	[Conditional("DEBUG")]
	public static void WriteLine(string format, params object[] args)
	{
		WriteLine(string.Format(null, format, args));
	}

	[Conditional("DEBUG")]
	public static void WriteLine(string message, string category)
	{
		if (category == null)
		{
			WriteLine(message);
		}
		else
		{
			WriteLine(category + ":" + message);
		}
	}

	[Conditional("DEBUG")]
	public static void Write(object value)
	{
		Write(value?.ToString());
	}

	[Conditional("DEBUG")]
	public static void Write(string message, string category)
	{
		if (category == null)
		{
			Write(message);
		}
		else
		{
			Write(category + ":" + message);
		}
	}

	[Conditional("DEBUG")]
	public static void Write(object value, string category)
	{
		Write(value?.ToString(), category);
	}

	[Conditional("DEBUG")]
	public static void WriteIf(bool condition, string message)
	{
		if (condition)
		{
			Write(message);
		}
	}

	[Conditional("DEBUG")]
	public static void WriteIf(bool condition, object value)
	{
		if (condition)
		{
			Write(value);
		}
	}

	[Conditional("DEBUG")]
	public static void WriteIf(bool condition, string message, string category)
	{
		if (condition)
		{
			Write(message, category);
		}
	}

	[Conditional("DEBUG")]
	public static void WriteIf(bool condition, object value, string category)
	{
		if (condition)
		{
			Write(value, category);
		}
	}

	[Conditional("DEBUG")]
	public static void WriteLineIf(bool condition, object value)
	{
		if (condition)
		{
			WriteLine(value);
		}
	}

	[Conditional("DEBUG")]
	public static void WriteLineIf(bool condition, object value, string category)
	{
		if (condition)
		{
			WriteLine(value, category);
		}
	}

	[Conditional("DEBUG")]
	public static void WriteLineIf(bool condition, string message)
	{
		if (condition)
		{
			WriteLine(message);
		}
	}

	[Conditional("DEBUG")]
	public static void WriteLineIf(bool condition, string message, string category)
	{
		if (condition)
		{
			WriteLine(message, category);
		}
	}

	private static string GetIndentString()
	{
		int num = IndentSize * IndentLevel;
		string text = s_indentString;
		if (text != null && text.Length == num)
		{
			return s_indentString;
		}
		return s_indentString = new string(' ', num);
	}

	private static void ShowAssertDialog(string stackTrace, string message, string detailMessage)
	{
	}

	private static void WriteCore(string message)
	{
	}
}

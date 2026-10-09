using System.Resources;
using Microsoft.Reflection;

namespace System.Diagnostics.Tracing.Internal;

internal static class Environment
{
	public static readonly string NewLine = System.Environment.NewLine;

	private static ResourceManager rm = new ResourceManager("Microsoft.Diagnostics.Tracing.Messages", typeof(Environment).Assembly());

	public static int TickCount => System.Environment.TickCount;

	public static string GetResourceString(string key, params object[] args)
	{
		string @string = rm.GetString(key);
		if (@string != null)
		{
			return string.Format(@string, args);
		}
		string text = string.Empty;
		foreach (object obj in args)
		{
			if (text != string.Empty)
			{
				text += ", ";
			}
			text += obj.ToString();
		}
		return key + " (" + text + ")";
	}

	public static string GetRuntimeResourceString(string key, params object[] args)
	{
		return GetResourceString(key, args);
	}
}

using System.Collections.Specialized;
using System.Reflection;

namespace System.Configuration;

public class AppSettingsReader
{
	private NameValueCollection appSettings;

	public AppSettingsReader()
	{
		appSettings = ConfigurationSettings.AppSettings;
	}

	public object GetValue(string key, Type type)
	{
		if (key == null)
		{
			throw new ArgumentNullException("key");
		}
		if (type == null)
		{
			throw new ArgumentNullException("type");
		}
		string text = appSettings[key];
		if (text == null)
		{
			throw new InvalidOperationException("'" + key + "' could not be found.");
		}
		if (type == typeof(string))
		{
			return text;
		}
		MethodInfo method = type.GetMethod("Parse", new Type[1] { typeof(string) });
		if (method == null)
		{
			throw new InvalidOperationException(string.Concat("Type ", type, " does not have a Parse method"));
		}
		object obj = null;
		try
		{
			return method.Invoke(null, new object[1] { text });
		}
		catch (Exception innerException)
		{
			throw new InvalidOperationException("Parse error.", innerException);
		}
	}
}

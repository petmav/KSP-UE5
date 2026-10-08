using System;
using System.Collections;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Security;
using System.Text;

namespace I18N.Common;

public class Manager
{
	private const string hex = "0123456789abcdef";

	private static Manager manager;

	private Hashtable handlers;

	private Hashtable active;

	private Hashtable assemblies;

	private static readonly object lockobj = new object();

	public static Manager PrimaryManager
	{
		get
		{
			lock (lockobj)
			{
				if (manager == null)
				{
					manager = new Manager();
				}
				return manager;
			}
		}
	}

	private Manager()
	{
		handlers = new Hashtable(CaseInsensitiveHashCodeProvider.Default, CaseInsensitiveComparer.Default);
		active = new Hashtable(16);
		assemblies = new Hashtable(8);
		LoadClassList();
	}

	private static string Normalize(string name)
	{
		return name.ToLower(CultureInfo.InvariantCulture).Replace('-', '_');
	}

	public Encoding GetEncoding(int codePage)
	{
		return Instantiate("CP" + codePage) as Encoding;
	}

	public Encoding GetEncoding(string name)
	{
		if (name == null)
		{
			return null;
		}
		string text = name;
		name = Normalize(name);
		Encoding encoding = Instantiate("ENC" + name) as Encoding;
		if (encoding == null)
		{
			encoding = Instantiate(name) as Encoding;
		}
		if (encoding == null)
		{
			string alias = Handlers.GetAlias(name);
			if (alias != null)
			{
				encoding = Instantiate("ENC" + alias) as Encoding;
				if (encoding == null)
				{
					encoding = Instantiate(alias) as Encoding;
				}
			}
		}
		if (encoding == null)
		{
			return null;
		}
		if (text.IndexOf('_') > 0 && encoding.WebName.IndexOf('-') > 0)
		{
			return null;
		}
		if (text.IndexOf('-') > 0 && encoding.WebName.IndexOf('_') > 0)
		{
			return null;
		}
		return encoding;
	}

	public CultureInfo GetCulture(int culture, bool useUserOverride)
	{
		StringBuilder stringBuilder = new StringBuilder();
		stringBuilder.Append("0123456789abcdef"[(culture >> 12) & 0xF]);
		stringBuilder.Append("0123456789abcdef"[(culture >> 8) & 0xF]);
		stringBuilder.Append("0123456789abcdef"[(culture >> 4) & 0xF]);
		stringBuilder.Append("0123456789abcdef"[culture & 0xF]);
		string text = stringBuilder.ToString();
		if (useUserOverride)
		{
			object obj = Instantiate("CIDO" + text);
			if (obj != null)
			{
				return obj as CultureInfo;
			}
		}
		return Instantiate("CID" + text) as CultureInfo;
	}

	public CultureInfo GetCulture(string name, bool useUserOverride)
	{
		if (name == null)
		{
			return null;
		}
		name = Normalize(name);
		if (useUserOverride)
		{
			object obj = Instantiate("CNO" + name.ToString());
			if (obj != null)
			{
				return obj as CultureInfo;
			}
		}
		return Instantiate("CN" + name.ToString()) as CultureInfo;
	}

	internal object Instantiate(string name)
	{
		lock (this)
		{
			object obj = active[name];
			if (obj != null)
			{
				return obj;
			}
			string text = (string)handlers[name];
			if (text == null)
			{
				return null;
			}
			Assembly assembly = (Assembly)assemblies[text];
			if ((object)assembly == null)
			{
				try
				{
					AssemblyName name2 = typeof(Manager).Assembly.GetName();
					name2.Name = text;
					assembly = Assembly.Load(name2);
				}
				catch (SystemException)
				{
					assembly = null;
				}
				if ((object)assembly == null)
				{
					return null;
				}
				assemblies[text] = assembly;
			}
			Type type = assembly.GetType(text + "." + name, throwOnError: false, ignoreCase: true);
			if ((object)type == null)
			{
				return null;
			}
			try
			{
				obj = type.InvokeMember(string.Empty, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.CreateInstance, null, null, null, null, null, null);
			}
			catch (MissingMethodException)
			{
				return null;
			}
			catch (SecurityException)
			{
				return null;
			}
			active.Add(name, obj);
			return obj;
		}
	}

	private void LoadClassList()
	{
		FileStream file;
		try
		{
			file = Assembly.GetExecutingAssembly().GetFile("I18N-handlers.def");
			if (file == null)
			{
				LoadInternalClasses();
				return;
			}
		}
		catch (FileLoadException)
		{
			LoadInternalClasses();
			return;
		}
		StreamReader streamReader = new StreamReader(file);
		string text;
		while ((text = streamReader.ReadLine()) != null)
		{
			if (text.Length == 0 || text[0] == '#')
			{
				continue;
			}
			int num = text.LastIndexOf('.');
			if (num != -1)
			{
				string key = text.Substring(num + 1);
				if (!handlers.Contains(key))
				{
					handlers.Add(key, text.Substring(0, num));
				}
			}
		}
		streamReader.Close();
	}

	private void LoadInternalClasses()
	{
		string[] list = Handlers.List;
		foreach (string text in list)
		{
			int num = text.LastIndexOf('.');
			if (num != -1)
			{
				string key = text.Substring(num + 1);
				if (!handlers.Contains(key))
				{
					handlers.Add(key, text.Substring(0, num));
				}
			}
		}
	}
}

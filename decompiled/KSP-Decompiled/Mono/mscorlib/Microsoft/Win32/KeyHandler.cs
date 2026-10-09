using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Security;
using System.Text;
using System.Threading;

namespace Microsoft.Win32;

internal class KeyHandler
{
	private static Hashtable key_to_handler;

	private static Hashtable dir_to_handler;

	private const string VolatileDirectoryName = "volatile-keys";

	public string Dir;

	private string ActualDir;

	public bool IsVolatile;

	private Hashtable values;

	private string file;

	private bool dirty;

	private static string user_store;

	private static string machine_store;

	public int ValueCount
	{
		get
		{
			lock (values)
			{
				return values.Keys.Count;
			}
		}
	}

	public bool IsMarkedForDeletion => !dir_to_handler.Contains(Dir);

	private static string UserStore
	{
		get
		{
			if (user_store == null)
			{
				user_store = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments), ".mono/registry");
			}
			return user_store;
		}
	}

	private static string MachineStore
	{
		get
		{
			if (machine_store == null)
			{
				machine_store = Environment.GetEnvironmentVariable("MONO_REGISTRY_PATH");
				if (machine_store == null)
				{
					string machineConfigPath = Environment.GetMachineConfigPath();
					int num = machineConfigPath.IndexOf("machine.config");
					machine_store = Path.Combine(Path.Combine(machineConfigPath.Substring(0, num - 1), ".."), "registry");
				}
			}
			return machine_store;
		}
	}

	static KeyHandler()
	{
		key_to_handler = new Hashtable(new RegistryKeyComparer());
		dir_to_handler = new Hashtable(new CaseInsensitiveHashCodeProvider(), new CaseInsensitiveComparer());
		CleanVolatileKeys();
	}

	private KeyHandler(RegistryKey rkey, string basedir)
		: this(rkey, basedir, is_volatile: false)
	{
	}

	private KeyHandler(RegistryKey rkey, string basedir, bool is_volatile)
	{
		string volatileDir = GetVolatileDir(basedir);
		string text = basedir;
		if (Directory.Exists(basedir))
		{
			is_volatile = false;
		}
		else if (Directory.Exists(volatileDir))
		{
			text = volatileDir;
			is_volatile = true;
		}
		else if (is_volatile)
		{
			text = volatileDir;
		}
		if (!Directory.Exists(text))
		{
			try
			{
				Directory.CreateDirectory(text);
			}
			catch (UnauthorizedAccessException inner)
			{
				throw new SecurityException("No access to the given key", inner);
			}
		}
		Dir = basedir;
		ActualDir = text;
		IsVolatile = is_volatile;
		file = Path.Combine(ActualDir, "values.xml");
		Load();
	}

	public void Load()
	{
		values = new Hashtable();
		if (!File.Exists(file))
		{
			return;
		}
		try
		{
			using FileStream stream = File.OpenRead(file);
			string text = new StreamReader(stream).ReadToEnd();
			if (text.Length == 0)
			{
				return;
			}
			SecurityElement securityElement = SecurityElement.FromString(text);
			if (!(securityElement.Tag == "values") || securityElement.Children == null)
			{
				return;
			}
			foreach (SecurityElement child in securityElement.Children)
			{
				if (child.Tag == "value")
				{
					LoadKey(child);
				}
			}
		}
		catch (UnauthorizedAccessException)
		{
			values.Clear();
			throw new SecurityException("No access to the given key");
		}
		catch (Exception arg)
		{
			Console.Error.WriteLine("While loading registry key at {0}: {1}", file, arg);
			values.Clear();
		}
	}

	private void LoadKey(SecurityElement se)
	{
		Hashtable attributes = se.Attributes;
		try
		{
			string text = (string)attributes["name"];
			if (text == null)
			{
				return;
			}
			string text2 = (string)attributes["type"];
			if (text2 == null)
			{
				return;
			}
			switch (text2)
			{
			case "int":
				values[text] = int.Parse(se.Text);
				break;
			case "bytearray":
				values[text] = Convert.FromBase64String(se.Text);
				break;
			case "string":
				values[text] = ((se.Text == null) ? string.Empty : se.Text);
				break;
			case "expand":
				values[text] = new ExpandString(se.Text);
				break;
			case "qword":
				values[text] = long.Parse(se.Text);
				break;
			case "string-array":
			{
				List<string> list = new List<string>();
				if (se.Children != null)
				{
					foreach (SecurityElement child in se.Children)
					{
						list.Add(child.Text);
					}
				}
				values[text] = list.ToArray();
				break;
			}
			}
		}
		catch
		{
		}
	}

	public RegistryKey Ensure(RegistryKey rkey, string extra, bool writable)
	{
		return Ensure(rkey, extra, writable, is_volatile: false);
	}

	public RegistryKey Ensure(RegistryKey rkey, string extra, bool writable, bool is_volatile)
	{
		lock (typeof(KeyHandler))
		{
			string text = Path.Combine(Dir, extra);
			KeyHandler keyHandler = (KeyHandler)dir_to_handler[text];
			if (keyHandler == null)
			{
				keyHandler = new KeyHandler(rkey, text, is_volatile);
			}
			RegistryKey registryKey = new RegistryKey(keyHandler, CombineName(rkey, extra), writable);
			key_to_handler[registryKey] = keyHandler;
			dir_to_handler[text] = keyHandler;
			return registryKey;
		}
	}

	public RegistryKey Probe(RegistryKey rkey, string extra, bool writable)
	{
		RegistryKey registryKey = null;
		lock (typeof(KeyHandler))
		{
			string text = Path.Combine(Dir, extra);
			KeyHandler keyHandler = (KeyHandler)dir_to_handler[text];
			if (keyHandler != null)
			{
				registryKey = new RegistryKey(keyHandler, CombineName(rkey, extra), writable);
				key_to_handler[registryKey] = keyHandler;
			}
			else if (Directory.Exists(text) || VolatileKeyExists(text))
			{
				keyHandler = new KeyHandler(rkey, text);
				registryKey = new RegistryKey(keyHandler, CombineName(rkey, extra), writable);
				dir_to_handler[text] = keyHandler;
				key_to_handler[registryKey] = keyHandler;
			}
			return registryKey;
		}
	}

	private static string CombineName(RegistryKey rkey, string extra)
	{
		if (extra.IndexOf('/') != -1)
		{
			extra = extra.Replace('/', '\\');
		}
		return rkey.Name + "\\" + extra;
	}

	private static long GetSystemBootTime()
	{
		if (!File.Exists("/proc/stat"))
		{
			return -1L;
		}
		string text = null;
		try
		{
			using StreamReader streamReader = new StreamReader("/proc/stat", Encoding.ASCII);
			string text2;
			while ((text2 = streamReader.ReadLine()) != null)
			{
				if (text2.StartsWith("btime"))
				{
					text = text2;
					break;
				}
			}
		}
		catch (Exception arg)
		{
			Console.Error.WriteLine("While reading system info {0}", arg);
		}
		if (text == null)
		{
			return -1L;
		}
		int num = text.IndexOf(' ');
		if (!long.TryParse(text.Substring(num, text.Length - num), out var result))
		{
			return -1L;
		}
		return result;
	}

	private static long GetRegisteredBootTime(string path)
	{
		if (!File.Exists(path))
		{
			return -1L;
		}
		string text = null;
		try
		{
			using StreamReader streamReader = new StreamReader(path, Encoding.ASCII);
			text = streamReader.ReadLine();
		}
		catch (Exception arg)
		{
			Console.Error.WriteLine("While reading registry data at {0}: {1}", path, arg);
		}
		if (text == null)
		{
			return -1L;
		}
		if (!long.TryParse(text, out var result))
		{
			return -1L;
		}
		return result;
	}

	private static void SaveRegisteredBootTime(string path, long btime)
	{
		try
		{
			using StreamWriter streamWriter = new StreamWriter(path, append: false, Encoding.ASCII);
			streamWriter.WriteLine(btime.ToString());
		}
		catch (Exception)
		{
		}
	}

	private static void CleanVolatileKeys()
	{
		long systemBootTime = GetSystemBootTime();
		string[] array = new string[2] { UserStore, MachineStore };
		foreach (string text in array)
		{
			if (!Directory.Exists(text))
			{
				continue;
			}
			string path = Path.Combine(text, "last-btime");
			string path2 = Path.Combine(text, "volatile-keys");
			if (Directory.Exists(path2))
			{
				long registeredBootTime = GetRegisteredBootTime(path);
				if (systemBootTime < 0 || registeredBootTime < 0 || registeredBootTime != systemBootTime)
				{
					Directory.Delete(path2, recursive: true);
				}
			}
			SaveRegisteredBootTime(path, systemBootTime);
		}
	}

	public static bool VolatileKeyExists(string dir)
	{
		lock (typeof(KeyHandler))
		{
			KeyHandler keyHandler = (KeyHandler)dir_to_handler[dir];
			if (keyHandler != null)
			{
				return keyHandler.IsVolatile;
			}
		}
		if (Directory.Exists(dir))
		{
			return false;
		}
		return Directory.Exists(GetVolatileDir(dir));
	}

	public static string GetVolatileDir(string dir)
	{
		string rootFromDir = GetRootFromDir(dir);
		return dir.Replace(rootFromDir, Path.Combine(rootFromDir, "volatile-keys"));
	}

	public static KeyHandler Lookup(RegistryKey rkey, bool createNonExisting)
	{
		lock (typeof(KeyHandler))
		{
			KeyHandler keyHandler = (KeyHandler)key_to_handler[rkey];
			if (keyHandler != null)
			{
				return keyHandler;
			}
			if (!rkey.IsRoot || !createNonExisting)
			{
				return null;
			}
			RegistryHive hive = rkey.Hive;
			switch (hive)
			{
			case RegistryHive.CurrentUser:
			{
				string text2 = Path.Combine(UserStore, hive.ToString());
				keyHandler = new KeyHandler(rkey, text2);
				dir_to_handler[text2] = keyHandler;
				break;
			}
			case RegistryHive.ClassesRoot:
			case RegistryHive.LocalMachine:
			case RegistryHive.Users:
			case RegistryHive.PerformanceData:
			case RegistryHive.CurrentConfig:
			case RegistryHive.DynData:
			{
				string text = Path.Combine(MachineStore, hive.ToString());
				keyHandler = new KeyHandler(rkey, text);
				dir_to_handler[text] = keyHandler;
				break;
			}
			default:
				throw new Exception("Unknown RegistryHive");
			}
			key_to_handler[rkey] = keyHandler;
			return keyHandler;
		}
	}

	private static string GetRootFromDir(string dir)
	{
		if (dir.IndexOf(UserStore) > -1)
		{
			return UserStore;
		}
		if (dir.IndexOf(MachineStore) > -1)
		{
			return MachineStore;
		}
		throw new Exception("Could not get root for dir " + dir);
	}

	public static void Drop(RegistryKey rkey)
	{
		lock (typeof(KeyHandler))
		{
			KeyHandler keyHandler = (KeyHandler)key_to_handler[rkey];
			if (keyHandler == null)
			{
				return;
			}
			key_to_handler.Remove(rkey);
			int num = 0;
			foreach (DictionaryEntry item in key_to_handler)
			{
				if (item.Value == keyHandler)
				{
					num++;
				}
			}
			if (num == 0)
			{
				dir_to_handler.Remove(keyHandler.Dir);
			}
		}
	}

	public static void Drop(string dir)
	{
		lock (typeof(KeyHandler))
		{
			KeyHandler keyHandler = (KeyHandler)dir_to_handler[dir];
			if (keyHandler == null)
			{
				return;
			}
			dir_to_handler.Remove(dir);
			ArrayList arrayList = new ArrayList();
			foreach (DictionaryEntry item in key_to_handler)
			{
				if (item.Value == keyHandler)
				{
					arrayList.Add(item.Key);
				}
			}
			foreach (object item2 in arrayList)
			{
				key_to_handler.Remove(item2);
			}
		}
	}

	public static bool Delete(string dir)
	{
		if (!Directory.Exists(dir))
		{
			string volatileDir = GetVolatileDir(dir);
			if (!Directory.Exists(volatileDir))
			{
				return false;
			}
			dir = volatileDir;
		}
		Directory.Delete(dir, recursive: true);
		Drop(dir);
		return true;
	}

	public RegistryValueKind GetValueKind(string name)
	{
		if (name == null)
		{
			return RegistryValueKind.Unknown;
		}
		object obj;
		lock (values)
		{
			obj = values[name];
		}
		if (obj == null)
		{
			return RegistryValueKind.Unknown;
		}
		if (obj is int)
		{
			return RegistryValueKind.DWord;
		}
		if (obj is string[])
		{
			return RegistryValueKind.MultiString;
		}
		if (obj is long)
		{
			return RegistryValueKind.QWord;
		}
		if (obj is byte[])
		{
			return RegistryValueKind.Binary;
		}
		if (obj is string)
		{
			return RegistryValueKind.String;
		}
		if (obj is ExpandString)
		{
			return RegistryValueKind.ExpandString;
		}
		return RegistryValueKind.Unknown;
	}

	public object GetValue(string name, RegistryValueOptions options)
	{
		if (IsMarkedForDeletion)
		{
			return null;
		}
		if (name == null)
		{
			name = string.Empty;
		}
		object obj;
		lock (values)
		{
			obj = values[name];
		}
		if (!(obj is ExpandString expandString))
		{
			return obj;
		}
		if ((options & RegistryValueOptions.DoNotExpandEnvironmentNames) == 0)
		{
			return expandString.Expand();
		}
		return expandString.ToString();
	}

	public void SetValue(string name, object value)
	{
		AssertNotMarkedForDeletion();
		if (name == null)
		{
			name = string.Empty;
		}
		lock (values)
		{
			if (value is int || value is string || value is byte[] || value is string[])
			{
				values[name] = value;
			}
			else
			{
				values[name] = value.ToString();
			}
		}
		SetDirty();
	}

	public string[] GetValueNames()
	{
		AssertNotMarkedForDeletion();
		lock (values)
		{
			ICollection keys = values.Keys;
			string[] array = new string[keys.Count];
			keys.CopyTo(array, 0);
			return array;
		}
	}

	public int GetSubKeyCount()
	{
		return GetSubKeyNames().Length;
	}

	public string[] GetSubKeyNames()
	{
		DirectoryInfo[] directories = new DirectoryInfo(ActualDir).GetDirectories();
		string[] array;
		if (IsVolatile || !Directory.Exists(GetVolatileDir(Dir)))
		{
			array = new string[directories.Length];
			for (int i = 0; i < directories.Length; i++)
			{
				DirectoryInfo directoryInfo = directories[i];
				array[i] = directoryInfo.Name;
			}
			return array;
		}
		DirectoryInfo[] directories2 = new DirectoryInfo(GetVolatileDir(Dir)).GetDirectories();
		Dictionary<string, string> dictionary = new Dictionary<string, string>();
		DirectoryInfo[] array2 = directories;
		foreach (DirectoryInfo directoryInfo2 in array2)
		{
			dictionary[directoryInfo2.Name] = directoryInfo2.Name;
		}
		array2 = directories2;
		foreach (DirectoryInfo directoryInfo3 in array2)
		{
			dictionary[directoryInfo3.Name] = directoryInfo3.Name;
		}
		array = new string[dictionary.Count];
		int num = 0;
		foreach (KeyValuePair<string, string> item in dictionary)
		{
			array[num++] = item.Value;
		}
		return array;
	}

	public void SetValue(string name, object value, RegistryValueKind valueKind)
	{
		SetDirty();
		if (name == null)
		{
			name = string.Empty;
		}
		lock (values)
		{
			switch (valueKind)
			{
			case RegistryValueKind.String:
				if (value is string)
				{
					values[name] = value;
					return;
				}
				break;
			case RegistryValueKind.ExpandString:
				if (value is string)
				{
					values[name] = new ExpandString((string)value);
					return;
				}
				break;
			case RegistryValueKind.Binary:
				if (value is byte[])
				{
					values[name] = value;
					return;
				}
				break;
			case RegistryValueKind.DWord:
				try
				{
					values[name] = Convert.ToInt32(value);
					return;
				}
				catch (OverflowException)
				{
				}
				break;
			case RegistryValueKind.MultiString:
				if (value is string[])
				{
					values[name] = value;
					return;
				}
				break;
			case RegistryValueKind.QWord:
				try
				{
					values[name] = Convert.ToInt64(value);
					return;
				}
				catch (OverflowException)
				{
				}
				break;
			default:
				throw new ArgumentException("unknown value", "valueKind");
			}
		}
		throw new ArgumentException("Value could not be converted to specified type", "valueKind");
	}

	private void SetDirty()
	{
		lock (typeof(KeyHandler))
		{
			if (!dirty)
			{
				dirty = true;
				new Timer(DirtyTimeout, null, 3000, -1);
			}
		}
	}

	public void DirtyTimeout(object state)
	{
		Flush();
	}

	public void Flush()
	{
		lock (typeof(KeyHandler))
		{
			if (dirty)
			{
				Save();
				dirty = false;
			}
		}
	}

	public bool ValueExists(string name)
	{
		if (name == null)
		{
			name = string.Empty;
		}
		lock (values)
		{
			return values.Contains(name);
		}
	}

	public void RemoveValue(string name)
	{
		AssertNotMarkedForDeletion();
		lock (values)
		{
			values.Remove(name);
		}
		SetDirty();
	}

	~KeyHandler()
	{
		Flush();
	}

	private void Save()
	{
		if (IsMarkedForDeletion)
		{
			return;
		}
		SecurityElement securityElement = new SecurityElement("values");
		lock (values)
		{
			if (!File.Exists(file) && values.Count == 0)
			{
				return;
			}
			foreach (DictionaryEntry value2 in values)
			{
				object value = value2.Value;
				SecurityElement securityElement2 = new SecurityElement("value");
				securityElement2.AddAttribute("name", SecurityElement.Escape((string)value2.Key));
				if (value is string)
				{
					securityElement2.AddAttribute("type", "string");
					securityElement2.Text = SecurityElement.Escape((string)value);
				}
				else if (value is int)
				{
					securityElement2.AddAttribute("type", "int");
					securityElement2.Text = value.ToString();
				}
				else if (value is long)
				{
					securityElement2.AddAttribute("type", "qword");
					securityElement2.Text = value.ToString();
				}
				else if (value is byte[])
				{
					securityElement2.AddAttribute("type", "bytearray");
					securityElement2.Text = Convert.ToBase64String((byte[])value);
				}
				else if (value is ExpandString)
				{
					securityElement2.AddAttribute("type", "expand");
					securityElement2.Text = SecurityElement.Escape(value.ToString());
				}
				else if (value is string[])
				{
					securityElement2.AddAttribute("type", "string-array");
					string[] array = (string[])value;
					foreach (string str in array)
					{
						SecurityElement securityElement3 = new SecurityElement("string");
						securityElement3.Text = SecurityElement.Escape(str);
						securityElement2.AddChild(securityElement3);
					}
				}
				securityElement.AddChild(securityElement2);
			}
		}
		using FileStream stream = File.Create(file);
		StreamWriter streamWriter = new StreamWriter(stream);
		streamWriter.Write(securityElement.ToString());
		streamWriter.Flush();
	}

	private void AssertNotMarkedForDeletion()
	{
		if (IsMarkedForDeletion)
		{
			throw RegistryKey.CreateMarkedForDeletionException();
		}
	}
}

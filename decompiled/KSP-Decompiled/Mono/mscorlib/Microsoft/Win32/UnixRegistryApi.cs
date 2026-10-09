using System;
using System.Globalization;
using System.IO;
using Microsoft.Win32.SafeHandles;

namespace Microsoft.Win32;

internal class UnixRegistryApi : IRegistryApi
{
	private static string ToUnix(string keyname)
	{
		if (keyname.IndexOf('\\') != -1)
		{
			keyname = keyname.Replace('\\', '/');
		}
		return keyname.ToLower();
	}

	private static bool IsWellKnownKey(string parentKeyName, string keyname)
	{
		if (parentKeyName == Registry.CurrentUser.Name || parentKeyName == Registry.LocalMachine.Name)
		{
			return string.Compare("software", keyname, ignoreCase: true, CultureInfo.InvariantCulture) == 0;
		}
		return false;
	}

	public RegistryKey CreateSubKey(RegistryKey rkey, string keyname)
	{
		return CreateSubKey(rkey, keyname, writable: true);
	}

	public RegistryKey CreateSubKey(RegistryKey rkey, string keyname, RegistryOptions options)
	{
		return CreateSubKey(rkey, keyname, writable: true, options == RegistryOptions.Volatile);
	}

	public RegistryKey OpenRemoteBaseKey(RegistryHive hKey, string machineName)
	{
		throw new NotImplementedException();
	}

	public RegistryKey OpenSubKey(RegistryKey rkey, string keyname, bool writable)
	{
		KeyHandler keyHandler = KeyHandler.Lookup(rkey, createNonExisting: true);
		if (keyHandler == null)
		{
			return null;
		}
		RegistryKey registryKey = keyHandler.Probe(rkey, ToUnix(keyname), writable);
		if (registryKey == null && IsWellKnownKey(rkey.Name, keyname))
		{
			registryKey = CreateSubKey(rkey, keyname, writable);
		}
		return registryKey;
	}

	public RegistryKey FromHandle(SafeRegistryHandle handle)
	{
		throw new NotImplementedException();
	}

	public void Flush(RegistryKey rkey)
	{
		KeyHandler.Lookup(rkey, createNonExisting: false)?.Flush();
	}

	public void Close(RegistryKey rkey)
	{
		KeyHandler.Drop(rkey);
	}

	public object GetValue(RegistryKey rkey, string name, object default_value, RegistryValueOptions options)
	{
		KeyHandler keyHandler = KeyHandler.Lookup(rkey, createNonExisting: true);
		if (keyHandler == null)
		{
			return default_value;
		}
		if (keyHandler.ValueExists(name))
		{
			return keyHandler.GetValue(name, options);
		}
		return default_value;
	}

	public void SetValue(RegistryKey rkey, string name, object value)
	{
		(KeyHandler.Lookup(rkey, createNonExisting: true) ?? throw RegistryKey.CreateMarkedForDeletionException()).SetValue(name, value);
	}

	public void SetValue(RegistryKey rkey, string name, object value, RegistryValueKind valueKind)
	{
		(KeyHandler.Lookup(rkey, createNonExisting: true) ?? throw RegistryKey.CreateMarkedForDeletionException()).SetValue(name, value, valueKind);
	}

	public int SubKeyCount(RegistryKey rkey)
	{
		return (KeyHandler.Lookup(rkey, createNonExisting: true) ?? throw RegistryKey.CreateMarkedForDeletionException()).GetSubKeyCount();
	}

	public int ValueCount(RegistryKey rkey)
	{
		return (KeyHandler.Lookup(rkey, createNonExisting: true) ?? throw RegistryKey.CreateMarkedForDeletionException()).ValueCount;
	}

	public void DeleteValue(RegistryKey rkey, string name, bool throw_if_missing)
	{
		KeyHandler keyHandler = KeyHandler.Lookup(rkey, createNonExisting: true);
		if (keyHandler != null)
		{
			if (throw_if_missing && !keyHandler.ValueExists(name))
			{
				throw new ArgumentException("the given value does not exist");
			}
			keyHandler.RemoveValue(name);
		}
	}

	public void DeleteKey(RegistryKey rkey, string keyname, bool throw_if_missing)
	{
		KeyHandler keyHandler = KeyHandler.Lookup(rkey, createNonExisting: true);
		if (keyHandler == null)
		{
			if (throw_if_missing)
			{
				throw new ArgumentException("the given value does not exist");
			}
		}
		else if (!KeyHandler.Delete(Path.Combine(keyHandler.Dir, ToUnix(keyname))) && throw_if_missing)
		{
			throw new ArgumentException("the given value does not exist");
		}
	}

	public string[] GetSubKeyNames(RegistryKey rkey)
	{
		return KeyHandler.Lookup(rkey, createNonExisting: true).GetSubKeyNames();
	}

	public string[] GetValueNames(RegistryKey rkey)
	{
		return (KeyHandler.Lookup(rkey, createNonExisting: true) ?? throw RegistryKey.CreateMarkedForDeletionException()).GetValueNames();
	}

	public string ToString(RegistryKey rkey)
	{
		return rkey.Name;
	}

	private RegistryKey CreateSubKey(RegistryKey rkey, string keyname, bool writable)
	{
		return CreateSubKey(rkey, keyname, writable, is_volatile: false);
	}

	private RegistryKey CreateSubKey(RegistryKey rkey, string keyname, bool writable, bool is_volatile)
	{
		KeyHandler obj = KeyHandler.Lookup(rkey, createNonExisting: true) ?? throw RegistryKey.CreateMarkedForDeletionException();
		if (KeyHandler.VolatileKeyExists(obj.Dir) && !is_volatile)
		{
			throw new IOException("Cannot create a non volatile subkey under a volatile key.");
		}
		return obj.Ensure(rkey, ToUnix(keyname), writable, is_volatile);
	}

	public RegistryValueKind GetValueKind(RegistryKey rkey, string name)
	{
		return KeyHandler.Lookup(rkey, createNonExisting: true)?.GetValueKind(name) ?? RegistryValueKind.Unknown;
	}

	public IntPtr GetHandle(RegistryKey key)
	{
		throw new NotImplementedException();
	}
}

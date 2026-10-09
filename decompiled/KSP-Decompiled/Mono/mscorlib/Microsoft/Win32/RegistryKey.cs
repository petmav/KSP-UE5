using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Security.AccessControl;
using System.Security.Permissions;
using System.Text;
using Microsoft.Win32.SafeHandles;
using Unity;

namespace Microsoft.Win32;

[ComVisible(true)]
public sealed class RegistryKey : MarshalByRefObject, IDisposable
{
	private object handle;

	private SafeRegistryHandle safe_handle;

	private object hive;

	private readonly string qname;

	private readonly bool isRemoteRoot;

	private readonly bool isWritable;

	private static readonly IRegistryApi RegistryApi;

	public string Name => qname;

	public int SubKeyCount
	{
		get
		{
			AssertKeyStillValid();
			return RegistryApi.SubKeyCount(this);
		}
	}

	public int ValueCount
	{
		get
		{
			AssertKeyStillValid();
			return RegistryApi.ValueCount(this);
		}
	}

	[MonoTODO("Not implemented in Unix")]
	[ComVisible(false)]
	public SafeRegistryHandle Handle
	{
		get
		{
			AssertKeyStillValid();
			if (safe_handle == null)
			{
				IntPtr preexistingHandle = RegistryApi.GetHandle(this);
				safe_handle = new SafeRegistryHandle(preexistingHandle, ownsHandle: true);
			}
			return safe_handle;
		}
	}

	[MonoLimitation("View is ignored in Mono.")]
	[ComVisible(false)]
	public RegistryView View => RegistryView.Default;

	internal bool IsRoot => hive != null;

	private bool IsWritable => isWritable;

	internal RegistryHive Hive
	{
		get
		{
			if (!IsRoot)
			{
				throw new NotSupportedException();
			}
			return (RegistryHive)hive;
		}
	}

	internal object InternalHandle => handle;

	static RegistryKey()
	{
		if (Path.DirectorySeparatorChar == '\\')
		{
			RegistryApi = new Win32RegistryApi();
		}
		else
		{
			RegistryApi = new UnixRegistryApi();
		}
	}

	internal RegistryKey(RegistryHive hiveId)
		: this(hiveId, new IntPtr((int)hiveId), remoteRoot: false)
	{
	}

	internal RegistryKey(RegistryHive hiveId, IntPtr keyHandle, bool remoteRoot)
	{
		hive = hiveId;
		handle = keyHandle;
		qname = GetHiveName(hiveId);
		isRemoteRoot = remoteRoot;
		isWritable = true;
	}

	internal RegistryKey(object data, string keyName, bool writable)
	{
		handle = data;
		qname = keyName;
		isWritable = writable;
	}

	internal static bool IsEquals(RegistryKey a, RegistryKey b)
	{
		if (a.hive == b.hive && a.handle == b.handle && a.qname == b.qname && a.isRemoteRoot == b.isRemoteRoot)
		{
			return a.isWritable == b.isWritable;
		}
		return false;
	}

	public void Dispose()
	{
		GC.SuppressFinalize(this);
		Close();
	}

	public void Flush()
	{
		RegistryApi.Flush(this);
	}

	public void Close()
	{
		Flush();
		if (isRemoteRoot || !IsRoot)
		{
			RegistryApi.Close(this);
			handle = null;
			safe_handle = null;
		}
	}

	public void SetValue(string name, object value)
	{
		AssertKeyStillValid();
		if (value == null)
		{
			throw new ArgumentNullException("value");
		}
		if (name != null)
		{
			AssertKeyNameLength(name);
		}
		if (!IsWritable)
		{
			throw new UnauthorizedAccessException("Cannot write to the registry key.");
		}
		RegistryApi.SetValue(this, name, value);
	}

	[ComVisible(false)]
	public void SetValue(string name, object value, RegistryValueKind valueKind)
	{
		AssertKeyStillValid();
		if (value == null)
		{
			throw new ArgumentNullException("value");
		}
		if (name != null)
		{
			AssertKeyNameLength(name);
		}
		if (!IsWritable)
		{
			throw new UnauthorizedAccessException("Cannot write to the registry key.");
		}
		RegistryApi.SetValue(this, name, value, valueKind);
	}

	public RegistryKey OpenSubKey(string name)
	{
		return OpenSubKey(name, writable: false);
	}

	public RegistryKey OpenSubKey(string name, bool writable)
	{
		AssertKeyStillValid();
		if (name == null)
		{
			throw new ArgumentNullException("name");
		}
		AssertKeyNameLength(name);
		return RegistryApi.OpenSubKey(this, name, writable);
	}

	public object GetValue(string name)
	{
		return GetValue(name, null);
	}

	public object GetValue(string name, object defaultValue)
	{
		AssertKeyStillValid();
		return RegistryApi.GetValue(this, name, defaultValue, RegistryValueOptions.None);
	}

	[ComVisible(false)]
	public object GetValue(string name, object defaultValue, RegistryValueOptions options)
	{
		AssertKeyStillValid();
		return RegistryApi.GetValue(this, name, defaultValue, options);
	}

	[ComVisible(false)]
	public RegistryValueKind GetValueKind(string name)
	{
		return RegistryApi.GetValueKind(this, name);
	}

	public RegistryKey CreateSubKey(string subkey)
	{
		AssertKeyStillValid();
		AssertKeyNameNotNull(subkey);
		AssertKeyNameLength(subkey);
		if (!IsWritable)
		{
			throw new UnauthorizedAccessException("Cannot write to the registry key.");
		}
		return RegistryApi.CreateSubKey(this, subkey);
	}

	[MonoLimitation("permissionCheck is ignored in Mono")]
	[ComVisible(false)]
	public RegistryKey CreateSubKey(string subkey, RegistryKeyPermissionCheck permissionCheck)
	{
		return CreateSubKey(subkey);
	}

	[ComVisible(false)]
	[MonoLimitation("permissionCheck and registrySecurity are ignored in Mono")]
	public RegistryKey CreateSubKey(string subkey, RegistryKeyPermissionCheck permissionCheck, RegistrySecurity registrySecurity)
	{
		return CreateSubKey(subkey);
	}

	[MonoLimitation("permissionCheck is ignored in Mono")]
	[ComVisible(false)]
	public RegistryKey CreateSubKey(string subkey, RegistryKeyPermissionCheck permissionCheck, RegistryOptions options)
	{
		AssertKeyStillValid();
		AssertKeyNameNotNull(subkey);
		AssertKeyNameLength(subkey);
		if (!IsWritable)
		{
			throw new UnauthorizedAccessException("Cannot write to the registry key.");
		}
		return RegistryApi.CreateSubKey(this, subkey, options);
	}

	[MonoLimitation("permissionCheck and registrySecurity are ignored in Mono")]
	[ComVisible(false)]
	public RegistryKey CreateSubKey(string subkey, RegistryKeyPermissionCheck permissionCheck, RegistryOptions registryOptions, RegistrySecurity registrySecurity)
	{
		return CreateSubKey(subkey, permissionCheck, registryOptions);
	}

	[ComVisible(false)]
	public RegistryKey CreateSubKey(string subkey, bool writable)
	{
		return CreateSubKey(subkey, (!writable) ? RegistryKeyPermissionCheck.ReadSubTree : RegistryKeyPermissionCheck.ReadWriteSubTree);
	}

	[ComVisible(false)]
	public RegistryKey CreateSubKey(string subkey, bool writable, RegistryOptions options)
	{
		return CreateSubKey(subkey, (!writable) ? RegistryKeyPermissionCheck.ReadSubTree : RegistryKeyPermissionCheck.ReadWriteSubTree, options);
	}

	public void DeleteSubKey(string subkey)
	{
		DeleteSubKey(subkey, throwOnMissingSubKey: true);
	}

	public void DeleteSubKey(string subkey, bool throwOnMissingSubKey)
	{
		AssertKeyStillValid();
		AssertKeyNameNotNull(subkey);
		AssertKeyNameLength(subkey);
		if (!IsWritable)
		{
			throw new UnauthorizedAccessException("Cannot write to the registry key.");
		}
		RegistryKey registryKey = OpenSubKey(subkey);
		if (registryKey == null)
		{
			if (throwOnMissingSubKey)
			{
				throw new ArgumentException("Cannot delete a subkey tree because the subkey does not exist.");
			}
			return;
		}
		if (registryKey.SubKeyCount > 0)
		{
			throw new InvalidOperationException("Registry key has subkeys and recursive removes are not supported by this method.");
		}
		registryKey.Close();
		RegistryApi.DeleteKey(this, subkey, throwOnMissingSubKey);
	}

	public void DeleteSubKeyTree(string subkey)
	{
		DeleteSubKeyTree(subkey, throwOnMissingSubKey: true);
	}

	public void DeleteSubKeyTree(string subkey, bool throwOnMissingSubKey)
	{
		AssertKeyStillValid();
		AssertKeyNameNotNull(subkey);
		AssertKeyNameLength(subkey);
		RegistryKey registryKey = OpenSubKey(subkey, writable: true);
		if (registryKey == null)
		{
			if (throwOnMissingSubKey)
			{
				throw new ArgumentException("Cannot delete a subkey tree because the subkey does not exist.");
			}
		}
		else
		{
			registryKey.DeleteChildKeysAndValues();
			registryKey.Close();
			DeleteSubKey(subkey, throwOnMissingSubKey: false);
		}
	}

	public void DeleteValue(string name)
	{
		DeleteValue(name, throwOnMissingValue: true);
	}

	public void DeleteValue(string name, bool throwOnMissingValue)
	{
		AssertKeyStillValid();
		if (name == null)
		{
			throw new ArgumentNullException("name");
		}
		if (!IsWritable)
		{
			throw new UnauthorizedAccessException("Cannot write to the registry key.");
		}
		RegistryApi.DeleteValue(this, name, throwOnMissingValue);
	}

	public RegistrySecurity GetAccessControl()
	{
		return GetAccessControl(AccessControlSections.Access | AccessControlSections.Owner | AccessControlSections.Group);
	}

	public RegistrySecurity GetAccessControl(AccessControlSections includeSections)
	{
		return new RegistrySecurity(Name, includeSections);
	}

	public string[] GetSubKeyNames()
	{
		AssertKeyStillValid();
		return RegistryApi.GetSubKeyNames(this);
	}

	public string[] GetValueNames()
	{
		AssertKeyStillValid();
		return RegistryApi.GetValueNames(this);
	}

	[ComVisible(false)]
	[MonoTODO("Not implemented on unix")]
	[SecurityPermission(SecurityAction.Demand, Flags = SecurityPermissionFlag.UnmanagedCode)]
	public static RegistryKey FromHandle(SafeRegistryHandle handle)
	{
		if (handle == null)
		{
			throw new ArgumentNullException("handle");
		}
		return RegistryApi.FromHandle(handle);
	}

	[ComVisible(false)]
	[MonoTODO("Not implemented on unix")]
	[SecurityPermission(SecurityAction.Demand, Flags = SecurityPermissionFlag.UnmanagedCode)]
	public static RegistryKey FromHandle(SafeRegistryHandle handle, RegistryView view)
	{
		return FromHandle(handle);
	}

	[MonoTODO("Not implemented on unix")]
	public static RegistryKey OpenRemoteBaseKey(RegistryHive hKey, string machineName)
	{
		if (machineName == null)
		{
			throw new ArgumentNullException("machineName");
		}
		return RegistryApi.OpenRemoteBaseKey(hKey, machineName);
	}

	[ComVisible(false)]
	[MonoTODO("Not implemented on unix")]
	public static RegistryKey OpenRemoteBaseKey(RegistryHive hKey, string machineName, RegistryView view)
	{
		if (machineName == null)
		{
			throw new ArgumentNullException("machineName");
		}
		return RegistryApi.OpenRemoteBaseKey(hKey, machineName);
	}

	[MonoLimitation("View is ignored in Mono")]
	[ComVisible(false)]
	public static RegistryKey OpenBaseKey(RegistryHive hKey, RegistryView view)
	{
		return hKey switch
		{
			RegistryHive.ClassesRoot => Registry.ClassesRoot, 
			RegistryHive.CurrentConfig => Registry.CurrentConfig, 
			RegistryHive.CurrentUser => Registry.CurrentUser, 
			RegistryHive.DynData => Registry.DynData, 
			RegistryHive.LocalMachine => Registry.LocalMachine, 
			RegistryHive.PerformanceData => Registry.PerformanceData, 
			RegistryHive.Users => Registry.Users, 
			_ => throw new ArgumentException("hKey"), 
		};
	}

	[ComVisible(false)]
	public RegistryKey OpenSubKey(string name, RegistryKeyPermissionCheck permissionCheck)
	{
		return OpenSubKey(name, permissionCheck == RegistryKeyPermissionCheck.ReadWriteSubTree);
	}

	[MonoLimitation("rights are ignored in Mono")]
	[ComVisible(false)]
	public RegistryKey OpenSubKey(string name, RegistryRights rights)
	{
		return OpenSubKey(name);
	}

	[ComVisible(false)]
	[MonoLimitation("rights are ignored in Mono")]
	public RegistryKey OpenSubKey(string name, RegistryKeyPermissionCheck permissionCheck, RegistryRights rights)
	{
		return OpenSubKey(name, permissionCheck == RegistryKeyPermissionCheck.ReadWriteSubTree);
	}

	public void SetAccessControl(RegistrySecurity registrySecurity)
	{
		if (registrySecurity == null)
		{
			throw new ArgumentNullException("registrySecurity");
		}
		registrySecurity.PersistModifications(Name);
	}

	public override string ToString()
	{
		AssertKeyStillValid();
		return RegistryApi.ToString(this);
	}

	private void AssertKeyStillValid()
	{
		if (handle == null)
		{
			throw new ObjectDisposedException("Microsoft.Win32.RegistryKey");
		}
	}

	private void AssertKeyNameNotNull(string subKeyName)
	{
		if (subKeyName == null)
		{
			throw new ArgumentNullException("name");
		}
	}

	private void AssertKeyNameLength(string name)
	{
		if (name.Length > 255)
		{
			throw new ArgumentException("Name of registry key cannot be greater than 255 characters");
		}
	}

	private void DeleteChildKeysAndValues()
	{
		if (!IsRoot)
		{
			string[] subKeyNames = GetSubKeyNames();
			foreach (string text in subKeyNames)
			{
				RegistryKey registryKey = OpenSubKey(text, writable: true);
				registryKey.DeleteChildKeysAndValues();
				registryKey.Close();
				DeleteSubKey(text, throwOnMissingSubKey: false);
			}
			subKeyNames = GetValueNames();
			foreach (string name in subKeyNames)
			{
				DeleteValue(name, throwOnMissingValue: false);
			}
		}
	}

	internal static string DecodeString(byte[] data)
	{
		string text = Encoding.Unicode.GetString(data);
		if (text.IndexOf('\0') != -1)
		{
			text = text.TrimEnd(default(char));
		}
		return text;
	}

	internal static IOException CreateMarkedForDeletionException()
	{
		throw new IOException("Illegal operation attempted on a registry key that has been marked for deletion.");
	}

	private static string GetHiveName(RegistryHive hive)
	{
		return hive switch
		{
			RegistryHive.ClassesRoot => "HKEY_CLASSES_ROOT", 
			RegistryHive.CurrentConfig => "HKEY_CURRENT_CONFIG", 
			RegistryHive.CurrentUser => "HKEY_CURRENT_USER", 
			RegistryHive.DynData => "HKEY_DYN_DATA", 
			RegistryHive.LocalMachine => "HKEY_LOCAL_MACHINE", 
			RegistryHive.PerformanceData => "HKEY_PERFORMANCE_DATA", 
			RegistryHive.Users => "HKEY_USERS", 
			_ => throw new NotImplementedException($"Registry hive '{hive.ToString()}' is not implemented."), 
		};
	}

	internal RegistryKey()
	{
		ThrowStub.ThrowNotSupportedException();
	}
}

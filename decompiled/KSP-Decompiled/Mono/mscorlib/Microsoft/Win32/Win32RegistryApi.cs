using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Security;
using System.Text;
using Microsoft.Win32.SafeHandles;

namespace Microsoft.Win32;

internal class Win32RegistryApi : IRegistryApi
{
	private const int OpenRegKeyRead = 131097;

	private const int OpenRegKeyWrite = 131078;

	private const int Int32ByteSize = 4;

	private const int Int64ByteSize = 8;

	private readonly int NativeBytesPerCharacter = Marshal.SystemDefaultCharSize;

	private const int RegOptionsNonVolatile = 0;

	private const int RegOptionsVolatile = 1;

	private const int MaxKeyLength = 255;

	private const int MaxValueLength = 16383;

	[DllImport("advapi32.dll", CharSet = CharSet.Unicode)]
	private static extern int RegCreateKeyEx(IntPtr keyBase, string keyName, int reserved, IntPtr lpClass, int options, int access, IntPtr securityAttrs, out IntPtr keyHandle, out int disposition);

	[DllImport("advapi32.dll", CharSet = CharSet.Unicode)]
	private static extern int RegCloseKey(IntPtr keyHandle);

	[DllImport("advapi32.dll", CharSet = CharSet.Unicode)]
	private static extern int RegConnectRegistry(string machineName, IntPtr hKey, out IntPtr keyHandle);

	[DllImport("advapi32.dll", CharSet = CharSet.Unicode)]
	private static extern int RegFlushKey(IntPtr keyHandle);

	[DllImport("advapi32.dll", CharSet = CharSet.Unicode)]
	private static extern int RegOpenKeyEx(IntPtr keyBase, string keyName, IntPtr reserved, int access, out IntPtr keyHandle);

	[DllImport("advapi32.dll", CharSet = CharSet.Unicode)]
	private static extern int RegDeleteKey(IntPtr keyHandle, string valueName);

	[DllImport("advapi32.dll", CharSet = CharSet.Unicode)]
	private static extern int RegDeleteValue(IntPtr keyHandle, string valueName);

	[DllImport("advapi32.dll", CharSet = CharSet.Unicode, EntryPoint = "RegEnumKeyExW")]
	internal unsafe static extern int RegEnumKeyEx(IntPtr keyHandle, int dwIndex, char* lpName, ref int lpcbName, int[] lpReserved, [Out] StringBuilder lpClass, int[] lpcbClass, long[] lpftLastWriteTime);

	[DllImport("advapi32.dll", CharSet = CharSet.Unicode)]
	internal unsafe static extern int RegEnumValue(IntPtr hKey, int dwIndex, char* lpValueName, ref int lpcbValueName, IntPtr lpReserved_MustBeZero, int[] lpType, byte[] lpData, int[] lpcbData);

	[DllImport("advapi32.dll", CharSet = CharSet.Unicode)]
	private static extern int RegSetValueEx(IntPtr keyBase, string valueName, IntPtr reserved, RegistryValueKind type, string data, int rawDataLength);

	[DllImport("advapi32.dll", CharSet = CharSet.Unicode)]
	private static extern int RegSetValueEx(IntPtr keyBase, string valueName, IntPtr reserved, RegistryValueKind type, byte[] rawData, int rawDataLength);

	[DllImport("advapi32.dll", CharSet = CharSet.Unicode)]
	private static extern int RegSetValueEx(IntPtr keyBase, string valueName, IntPtr reserved, RegistryValueKind type, ref int data, int rawDataLength);

	[DllImport("advapi32.dll", CharSet = CharSet.Unicode)]
	private static extern int RegSetValueEx(IntPtr keyBase, string valueName, IntPtr reserved, RegistryValueKind type, ref long data, int rawDataLength);

	[DllImport("advapi32.dll", CharSet = CharSet.Unicode)]
	private static extern int RegQueryValueEx(IntPtr keyBase, string valueName, IntPtr reserved, ref RegistryValueKind type, IntPtr zero, ref int dataSize);

	[DllImport("advapi32.dll", CharSet = CharSet.Unicode)]
	private static extern int RegQueryValueEx(IntPtr keyBase, string valueName, IntPtr reserved, ref RegistryValueKind type, [Out] byte[] data, ref int dataSize);

	[DllImport("advapi32.dll", CharSet = CharSet.Unicode)]
	private static extern int RegQueryValueEx(IntPtr keyBase, string valueName, IntPtr reserved, ref RegistryValueKind type, ref int data, ref int dataSize);

	[DllImport("advapi32.dll", CharSet = CharSet.Unicode)]
	private static extern int RegQueryValueEx(IntPtr keyBase, string valueName, IntPtr reserved, ref RegistryValueKind type, ref long data, ref int dataSize);

	[DllImport("advapi32.dll", CharSet = CharSet.Unicode, EntryPoint = "RegQueryInfoKeyW")]
	internal static extern int RegQueryInfoKey(IntPtr hKey, [Out] StringBuilder lpClass, int[] lpcbClass, IntPtr lpReserved_MustBeZero, ref int lpcSubKeys, int[] lpcbMaxSubKeyLen, int[] lpcbMaxClassLen, ref int lpcValues, int[] lpcbMaxValueNameLen, int[] lpcbMaxValueLen, int[] lpcbSecurityDescriptor, int[] lpftLastWriteTime);

	public IntPtr GetHandle(RegistryKey key)
	{
		return (IntPtr)key.InternalHandle;
	}

	private static bool IsHandleValid(RegistryKey key)
	{
		return key.InternalHandle != null;
	}

	public RegistryValueKind GetValueKind(RegistryKey rkey, string name)
	{
		RegistryValueKind type = RegistryValueKind.Unknown;
		int dataSize = 0;
		int num = RegQueryValueEx(GetHandle(rkey), name, IntPtr.Zero, ref type, IntPtr.Zero, ref dataSize);
		if (num == 2 || num == 1018)
		{
			return RegistryValueKind.Unknown;
		}
		return type;
	}

	public object GetValue(RegistryKey rkey, string name, object defaultValue, RegistryValueOptions options)
	{
		RegistryValueKind type = RegistryValueKind.Unknown;
		int dataSize = 0;
		object obj = null;
		IntPtr handle = GetHandle(rkey);
		int num = RegQueryValueEx(handle, name, IntPtr.Zero, ref type, IntPtr.Zero, ref dataSize);
		switch (num)
		{
		case 2:
		case 1018:
			return defaultValue;
		default:
			GenerateException(num);
			break;
		case 0:
		case 234:
			break;
		}
		switch (type)
		{
		case RegistryValueKind.String:
		{
			num = GetBinaryValue(rkey, name, type, out var data2, dataSize);
			obj = RegistryKey.DecodeString(data2);
			break;
		}
		case RegistryValueKind.ExpandString:
		{
			num = GetBinaryValue(rkey, name, type, out var data6, dataSize);
			obj = RegistryKey.DecodeString(data6);
			if ((options & RegistryValueOptions.DoNotExpandEnvironmentNames) == 0)
			{
				obj = Environment.ExpandEnvironmentVariables((string)obj);
			}
			break;
		}
		case RegistryValueKind.DWord:
		{
			int data5 = 0;
			num = RegQueryValueEx(handle, name, IntPtr.Zero, ref type, ref data5, ref dataSize);
			obj = data5;
			break;
		}
		case RegistryValueKind.QWord:
		{
			long data4 = 0L;
			num = RegQueryValueEx(handle, name, IntPtr.Zero, ref type, ref data4, ref dataSize);
			obj = data4;
			break;
		}
		case RegistryValueKind.Binary:
		{
			num = GetBinaryValue(rkey, name, type, out var data3, dataSize);
			obj = data3;
			break;
		}
		case RegistryValueKind.MultiString:
		{
			obj = null;
			num = GetBinaryValue(rkey, name, type, out var data, dataSize);
			if (num == 0)
			{
				obj = RegistryKey.DecodeString(data).Split(default(char));
			}
			break;
		}
		default:
			throw new SystemException();
		}
		if (num != 0)
		{
			GenerateException(num);
		}
		return obj;
	}

	public void SetValue(RegistryKey rkey, string name, object value, RegistryValueKind valueKind)
	{
		Type type = value.GetType();
		IntPtr handle = GetHandle(rkey);
		switch (valueKind)
		{
		case RegistryValueKind.QWord:
			try
			{
				long data = Convert.ToInt64(value);
				CheckResult(RegSetValueEx(handle, name, IntPtr.Zero, RegistryValueKind.QWord, ref data, 8));
				return;
			}
			catch (OverflowException)
			{
			}
			break;
		case RegistryValueKind.DWord:
			try
			{
				int data2 = Convert.ToInt32(value);
				CheckResult(RegSetValueEx(handle, name, IntPtr.Zero, RegistryValueKind.DWord, ref data2, 4));
				return;
			}
			catch (OverflowException)
			{
			}
			break;
		case RegistryValueKind.Binary:
			if (type == typeof(byte[]))
			{
				byte[] array2 = (byte[])value;
				CheckResult(RegSetValueEx(handle, name, IntPtr.Zero, RegistryValueKind.Binary, array2, array2.Length));
				return;
			}
			break;
		case RegistryValueKind.MultiString:
			if (type == typeof(string[]))
			{
				string[] obj = (string[])value;
				StringBuilder stringBuilder = new StringBuilder();
				string[] array = obj;
				foreach (string value2 in array)
				{
					stringBuilder.Append(value2);
					stringBuilder.Append('\0');
				}
				stringBuilder.Append('\0');
				byte[] bytes = Encoding.Unicode.GetBytes(stringBuilder.ToString());
				CheckResult(RegSetValueEx(handle, name, IntPtr.Zero, RegistryValueKind.MultiString, bytes, bytes.Length));
				return;
			}
			break;
		case RegistryValueKind.String:
		case RegistryValueKind.ExpandString:
			if (type == typeof(string))
			{
				string text = $"{value}{'\0'}";
				CheckResult(RegSetValueEx(handle, name, IntPtr.Zero, valueKind, text, text.Length * NativeBytesPerCharacter));
				return;
			}
			break;
		default:
			if (type.IsArray)
			{
				throw new ArgumentException("Only string and byte arrays can written as registry values");
			}
			break;
		}
		throw new ArgumentException("Type does not match the valueKind");
	}

	public void SetValue(RegistryKey rkey, string name, object value)
	{
		Type type = value.GetType();
		IntPtr handle = GetHandle(rkey);
		int num;
		if (type == typeof(int))
		{
			int data = (int)value;
			num = RegSetValueEx(handle, name, IntPtr.Zero, RegistryValueKind.DWord, ref data, 4);
		}
		else if (type == typeof(byte[]))
		{
			byte[] array = (byte[])value;
			num = RegSetValueEx(handle, name, IntPtr.Zero, RegistryValueKind.Binary, array, array.Length);
		}
		else if (type == typeof(string[]))
		{
			string[] obj = (string[])value;
			StringBuilder stringBuilder = new StringBuilder();
			string[] array2 = obj;
			foreach (string value2 in array2)
			{
				stringBuilder.Append(value2);
				stringBuilder.Append('\0');
			}
			stringBuilder.Append('\0');
			byte[] bytes = Encoding.Unicode.GetBytes(stringBuilder.ToString());
			num = RegSetValueEx(handle, name, IntPtr.Zero, RegistryValueKind.MultiString, bytes, bytes.Length);
		}
		else
		{
			if (type.IsArray)
			{
				throw new ArgumentException("Only string and byte arrays can written as registry values");
			}
			string text = $"{value}{'\0'}";
			num = RegSetValueEx(handle, name, IntPtr.Zero, RegistryValueKind.String, text, text.Length * NativeBytesPerCharacter);
		}
		if (num != 0)
		{
			GenerateException(num);
		}
	}

	private int GetBinaryValue(RegistryKey rkey, string name, RegistryValueKind type, out byte[] data, int size)
	{
		byte[] array = new byte[size];
		int result = RegQueryValueEx(GetHandle(rkey), name, IntPtr.Zero, ref type, array, ref size);
		data = array;
		return result;
	}

	public int SubKeyCount(RegistryKey rkey)
	{
		int lpcSubKeys = 0;
		int lpcValues = 0;
		int num = RegQueryInfoKey(GetHandle(rkey), null, null, IntPtr.Zero, ref lpcSubKeys, null, null, ref lpcValues, null, null, null, null);
		if (num != 0)
		{
			GenerateException(num);
		}
		return lpcSubKeys;
	}

	public int ValueCount(RegistryKey rkey)
	{
		int lpcValues = 0;
		int lpcSubKeys = 0;
		int num = RegQueryInfoKey(GetHandle(rkey), null, null, IntPtr.Zero, ref lpcSubKeys, null, null, ref lpcValues, null, null, null, null);
		if (num != 0)
		{
			GenerateException(num);
		}
		return lpcValues;
	}

	public RegistryKey OpenRemoteBaseKey(RegistryHive hKey, string machineName)
	{
		IntPtr hKey2 = new IntPtr((int)hKey);
		IntPtr keyHandle;
		int num = RegConnectRegistry(machineName, hKey2, out keyHandle);
		if (num != 0)
		{
			GenerateException(num);
		}
		return new RegistryKey(hKey, keyHandle, remoteRoot: true);
	}

	public RegistryKey OpenSubKey(RegistryKey rkey, string keyName, bool writable)
	{
		int num = 131097;
		if (writable)
		{
			num |= 0x20006;
		}
		IntPtr keyHandle;
		int num2 = RegOpenKeyEx(GetHandle(rkey), keyName, IntPtr.Zero, num, out keyHandle);
		switch (num2)
		{
		case 2:
		case 1018:
			return null;
		default:
			GenerateException(num2);
			break;
		case 0:
			break;
		}
		return new RegistryKey(keyHandle, CombineName(rkey, keyName), writable);
	}

	public void Flush(RegistryKey rkey)
	{
		if (IsHandleValid(rkey))
		{
			RegFlushKey(GetHandle(rkey));
		}
	}

	public void Close(RegistryKey rkey)
	{
		if (IsHandleValid(rkey))
		{
			SafeRegistryHandle handle = rkey.Handle;
			if (handle != null)
			{
				handle.Close();
			}
			else
			{
				RegCloseKey(GetHandle(rkey));
			}
		}
	}

	public RegistryKey FromHandle(SafeRegistryHandle handle)
	{
		return new RegistryKey(handle.DangerousGetHandle(), string.Empty, writable: true);
	}

	public RegistryKey CreateSubKey(RegistryKey rkey, string keyName)
	{
		IntPtr keyHandle;
		int disposition;
		int num = RegCreateKeyEx(GetHandle(rkey), keyName, 0, IntPtr.Zero, 0, 131103, IntPtr.Zero, out keyHandle, out disposition);
		if (num != 0)
		{
			GenerateException(num);
		}
		return new RegistryKey(keyHandle, CombineName(rkey, keyName), writable: true);
	}

	public RegistryKey CreateSubKey(RegistryKey rkey, string keyName, RegistryOptions options)
	{
		IntPtr keyHandle;
		int disposition;
		int num = RegCreateKeyEx(GetHandle(rkey), keyName, 0, IntPtr.Zero, (options == RegistryOptions.Volatile) ? 1 : 0, 131103, IntPtr.Zero, out keyHandle, out disposition);
		if (num != 0)
		{
			GenerateException(num);
		}
		return new RegistryKey(keyHandle, CombineName(rkey, keyName), writable: true);
	}

	public void DeleteKey(RegistryKey rkey, string keyName, bool shouldThrowWhenKeyMissing)
	{
		int num = RegDeleteKey(GetHandle(rkey), keyName);
		switch (num)
		{
		case 2:
			if (shouldThrowWhenKeyMissing)
			{
				throw new ArgumentException("key " + keyName);
			}
			break;
		default:
			GenerateException(num);
			break;
		case 0:
			break;
		}
	}

	public void DeleteValue(RegistryKey rkey, string value, bool shouldThrowWhenKeyMissing)
	{
		int num = RegDeleteValue(GetHandle(rkey), value);
		switch (num)
		{
		case 0:
		case 1018:
			break;
		case 2:
			if (shouldThrowWhenKeyMissing)
			{
				throw new ArgumentException("value " + value);
			}
			break;
		default:
			GenerateException(num);
			break;
		}
	}

	public unsafe string[] GetSubKeyNames(RegistryKey rkey)
	{
		int num = SubKeyCount(rkey);
		string[] array = new string[num];
		if (num > 0)
		{
			IntPtr handle = GetHandle(rkey);
			char[] array2 = new char[256];
			fixed (char* ptr = &array2[0])
			{
				for (int i = 0; i < num; i++)
				{
					int lpcbName = array2.Length;
					int num2 = RegEnumKeyEx(handle, i, ptr, ref lpcbName, null, null, null, null);
					if (num2 != 0)
					{
						GenerateException(num2);
					}
					array[i] = new string(ptr);
				}
			}
		}
		return array;
	}

	public unsafe string[] GetValueNames(RegistryKey rkey)
	{
		int num = ValueCount(rkey);
		string[] array = new string[num];
		if (num > 0)
		{
			IntPtr handle = GetHandle(rkey);
			char[] array2 = new char[16384];
			fixed (char* ptr = &array2[0])
			{
				for (int i = 0; i < num; i++)
				{
					int lpcbValueName = array2.Length;
					int num2 = RegEnumValue(handle, i, ptr, ref lpcbValueName, IntPtr.Zero, null, null, null);
					if (num2 != 0 && num2 != 234)
					{
						GenerateException(num2);
					}
					array[i] = new string(ptr);
				}
			}
		}
		return array;
	}

	private void CheckResult(int result)
	{
		if (result != 0)
		{
			GenerateException(result);
		}
	}

	private void GenerateException(int errorCode)
	{
		switch (errorCode)
		{
		case 2:
		case 87:
			throw new ArgumentException();
		case 5:
			throw new SecurityException();
		case 53:
			throw new IOException("The network path was not found.");
		case 6:
			throw new IOException("Invalid handle.");
		case 1018:
			throw RegistryKey.CreateMarkedForDeletionException();
		case 1021:
			throw new IOException("Cannot create a stable subkey under a volatile parent key.");
		default:
			throw new SystemException();
		}
	}

	public string ToString(RegistryKey rkey)
	{
		return rkey.Name;
	}

	internal static string CombineName(RegistryKey rkey, string localName)
	{
		return rkey.Name + "\\" + localName;
	}
}

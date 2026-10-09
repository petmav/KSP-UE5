using System.Collections;

namespace Microsoft.Win32;

internal class RegistryKeyComparer : IEqualityComparer
{
	public new bool Equals(object x, object y)
	{
		return RegistryKey.IsEquals((RegistryKey)x, (RegistryKey)y);
	}

	public int GetHashCode(object obj)
	{
		return ((RegistryKey)obj).Name?.GetHashCode() ?? 0;
	}
}

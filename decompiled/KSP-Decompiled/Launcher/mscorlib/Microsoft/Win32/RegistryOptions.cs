using System;

namespace Microsoft.Win32;

[Serializable]
[Flags]
public enum RegistryOptions
{
	None = 0,
	Volatile = 1
}

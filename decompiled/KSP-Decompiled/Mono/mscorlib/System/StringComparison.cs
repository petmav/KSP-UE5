using System.Runtime.InteropServices;

namespace System;

[Serializable]
[ComVisible(true)]
public enum StringComparison
{
	CurrentCulture,
	CurrentCultureIgnoreCase,
	InvariantCulture,
	InvariantCultureIgnoreCase,
	Ordinal,
	OrdinalIgnoreCase
}

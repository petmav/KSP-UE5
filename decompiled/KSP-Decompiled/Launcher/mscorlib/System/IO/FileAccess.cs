using System.Runtime.InteropServices;

namespace System.IO;

[Serializable]
[ComVisible(true)]
[Flags]
public enum FileAccess
{
	Read = 1,
	Write = 2,
	ReadWrite = 3
}

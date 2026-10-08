using System.Runtime.InteropServices;

namespace System.IO;

[Serializable]
[ComVisible(true)]
[Flags]
public enum FileOptions
{
	None = 0,
	Encrypted = 0x4000,
	DeleteOnClose = 0x4000000,
	SequentialScan = 0x8000000,
	RandomAccess = 0x10000000,
	Asynchronous = 0x40000000,
	WriteThrough = int.MinValue
}

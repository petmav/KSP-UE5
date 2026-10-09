using System.Runtime.InteropServices;

namespace System.IO;

[Serializable]
[ComVisible(true)]
public enum SeekOrigin
{
	Begin,
	Current,
	End
}

using System.ComponentModel;

namespace System.Data;

[EditorBrowsable(EditorBrowsableState.Never)]
[Obsolete("PropertyAttributes has been deprecated.  http://go.microsoft.com/fwlink/?linkid=14202")]
[Flags]
public enum PropertyAttributes
{
	NotSupported = 0,
	Required = 1,
	Optional = 2,
	Read = 0x200,
	Write = 0x400
}

using System.Runtime.InteropServices;

namespace System.EnterpriseServices;

[Serializable]
[ComVisible(false)]
public enum PropertyReleaseMode
{
	Process = 1,
	Standard = 0
}

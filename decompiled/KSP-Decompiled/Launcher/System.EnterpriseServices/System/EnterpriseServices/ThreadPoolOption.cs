using System.Runtime.InteropServices;

namespace System.EnterpriseServices;

[Serializable]
[ComVisible(false)]
public enum ThreadPoolOption
{
	Inherit = 1,
	MTA = 3,
	None = 0,
	STA = 2
}

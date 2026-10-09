using System.Runtime.InteropServices;

namespace System.EnterpriseServices;

[Serializable]
[ComVisible(false)]
public enum PropertyLockMode
{
	Method = 1,
	SetGet = 0
}

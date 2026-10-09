using System.Runtime.InteropServices;

namespace System.EnterpriseServices;

[Serializable]
[ComVisible(false)]
public enum BindingOption
{
	BindingToPoolThread = 1,
	NoBinding = 0
}

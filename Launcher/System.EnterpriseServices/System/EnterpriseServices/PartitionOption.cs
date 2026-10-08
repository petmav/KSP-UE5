using System.Runtime.InteropServices;

namespace System.EnterpriseServices;

[Serializable]
[ComVisible(false)]
public enum PartitionOption
{
	Ignore,
	Inherit,
	New
}

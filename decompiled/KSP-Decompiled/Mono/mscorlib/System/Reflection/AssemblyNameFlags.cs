using System.Runtime.InteropServices;

namespace System.Reflection;

[Serializable]
[ComVisible(true)]
[Flags]
public enum AssemblyNameFlags
{
	None = 0,
	PublicKey = 1,
	EnableJITcompileOptimizer = 0x4000,
	EnableJITcompileTracking = 0x8000,
	Retargetable = 0x100
}

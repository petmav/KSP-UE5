using System.Runtime.InteropServices;

namespace System.Reflection;

[Serializable]
[ComVisible(true)]
public enum ProcessorArchitecture
{
	None,
	MSIL,
	X86,
	IA64,
	Amd64,
	Arm
}

using System.Runtime.InteropServices;

namespace System.Reflection;

[Serializable]
[ComVisible(true)]
[Flags]
public enum ResourceLocation
{
	Embedded = 1,
	ContainedInAnotherAssembly = 2,
	ContainedInManifestFile = 4
}

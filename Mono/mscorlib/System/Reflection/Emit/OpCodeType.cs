using System.Runtime.InteropServices;

namespace System.Reflection.Emit;

[Serializable]
[ComVisible(true)]
public enum OpCodeType
{
	[Obsolete("This API has been deprecated.")]
	Annotation,
	Macro,
	Nternal,
	Objmodel,
	Prefix,
	Primitive
}

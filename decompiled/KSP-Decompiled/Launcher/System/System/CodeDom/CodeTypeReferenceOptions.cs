using System.Runtime.InteropServices;

namespace System.CodeDom;

[Serializable]
[Flags]
[ComVisible(true)]
public enum CodeTypeReferenceOptions
{
	GenericTypeParameter = 2,
	GlobalReference = 1
}

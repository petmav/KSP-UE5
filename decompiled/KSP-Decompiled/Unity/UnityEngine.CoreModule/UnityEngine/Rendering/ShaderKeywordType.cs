using UnityEngine.Bindings;
using UnityEngine.Scripting;

namespace UnityEngine.Rendering;

[NativeHeader("Runtime/Shaders/ShaderKeywordSet.h")]
[UsedByNativeCode]
public enum ShaderKeywordType
{
	None = 0,
	BuiltinDefault = 2,
	BuiltinExtra = 6,
	BuiltinAutoStripped = 10,
	UserDefined = 16
}

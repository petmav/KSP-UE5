using UnityEngine.Bindings;
using UnityEngine.Scripting;

namespace UnityEngine.Rendering;

[UsedByNativeCode]
[NativeHeader("Runtime/Graphics/AsyncGPUReadbackManaged.h")]
internal struct AsyncRequestNativeArrayData
{
	public unsafe void* nativeArrayBuffer;

	public long lengthInBytes;
}

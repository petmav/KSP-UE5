using System.Runtime.CompilerServices;
using UnityEngine.Bindings;

namespace UnityEngine.XR.WSA;

[StaticAccessor("HolographicEmulation::HolographicEmulationManager::Get()", StaticAccessorType.Dot)]
[NativeHeader("Modules/VR/HoloLens/HolographicEmulation/HolographicEmulationManager.h")]
internal class HolographicEmulationHelper
{
	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeConditional("ENABLE_HOLOLENS_MODULE", StubReturnStatement = "HolographicEmulation::EmulationMode_None")]
	[NativeName("GetEmulationMode")]
	internal static extern EmulationMode GetEmulationMode();
}

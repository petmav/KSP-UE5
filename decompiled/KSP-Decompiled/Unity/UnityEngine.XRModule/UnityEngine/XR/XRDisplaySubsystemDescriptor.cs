using System.Runtime.CompilerServices;
using UnityEngine.Bindings;
using UnityEngine.Scripting;

namespace UnityEngine.XR;

[NativeType(Header = "Modules/XR/Subsystems/Display/XRDisplaySubsystemDescriptor.h")]
[UsedByNativeCode]
public class XRDisplaySubsystemDescriptor : IntegratedSubsystemDescriptor<XRDisplaySubsystem>
{
	[NativeConditional("ENABLE_XR")]
	public extern bool disablesLegacyVr
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod("TryGetAvailableMirrorModeCount")]
	[NativeConditional("ENABLE_XR")]
	public extern int GetAvailableMirrorBlitModeCount();

	[MethodImpl(MethodImplOptions.InternalCall)]
	[NativeMethod("TryGetMirrorModeByIndex")]
	[NativeConditional("ENABLE_XR")]
	public extern void GetMirrorBlitModeByIndex(int index, out XRMirrorViewBlitModeDesc mode);
}

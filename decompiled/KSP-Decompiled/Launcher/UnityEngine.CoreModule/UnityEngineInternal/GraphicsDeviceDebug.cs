using System.Runtime.CompilerServices;
using UnityEngine.Bindings;

namespace UnityEngineInternal;

[StaticAccessor("GraphicsDeviceDebug", StaticAccessorType.DoubleColon)]
[NativeHeader("Runtime/Export/Graphics/GraphicsDeviceDebug.bindings.h")]
internal static class GraphicsDeviceDebug
{
	internal static GraphicsDeviceDebugSettings settings
	{
		get
		{
			get_settings_Injected(out var ret);
			return ret;
		}
		set
		{
			set_settings_Injected(ref value);
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SpecialName]
	private static extern void get_settings_Injected(out GraphicsDeviceDebugSettings ret);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SpecialName]
	private static extern void set_settings_Injected(ref GraphicsDeviceDebugSettings value);
}

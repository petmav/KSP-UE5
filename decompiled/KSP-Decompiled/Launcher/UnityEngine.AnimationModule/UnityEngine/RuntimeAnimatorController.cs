using System.Runtime.CompilerServices;
using UnityEngine.Bindings;
using UnityEngine.Scripting;

namespace UnityEngine;

[NativeHeader("Modules/Animation/RuntimeAnimatorController.h")]
[UsedByNativeCode]
[ExcludeFromObjectFactory]
public class RuntimeAnimatorController : Object
{
	public extern AnimationClip[] animationClips
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
	}

	protected RuntimeAnimatorController()
	{
	}
}

using System;
using UnityEngine.Bindings;
using UnityEngine.Scripting;

namespace UnityEngine.VFX;

[NativeHeader("Modules/VFX/Public/ScriptBindings/VisualEffectAssetBindings.h")]
[UsedByNativeCode]
public struct VFXExposedProperty
{
	public string name;

	public Type type;
}

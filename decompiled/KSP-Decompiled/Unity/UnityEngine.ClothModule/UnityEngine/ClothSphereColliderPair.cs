using UnityEngine.Bindings;
using UnityEngine.Scripting;

namespace UnityEngine;

[NativeHeader("Modules/Cloth/Cloth.h")]
[UsedByNativeCode]
public struct ClothSphereColliderPair
{
	public SphereCollider first { get; set; }

	public SphereCollider second { get; set; }

	public ClothSphereColliderPair(SphereCollider a)
	{
		first = a;
		second = null;
	}

	public ClothSphereColliderPair(SphereCollider a, SphereCollider b)
	{
		first = a;
		second = b;
	}
}

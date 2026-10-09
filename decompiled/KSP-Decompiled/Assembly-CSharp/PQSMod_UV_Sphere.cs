using UnityEngine;

[AddComponentMenu("PQuadSphere/Mods/Material/Quad Sphere UV")]
public class PQSMod_UV_Sphere : PQSMod
{
	public override void OnSetup()
	{
		requirements = PQS.ModiferRequirements.UVSphereCoords;
	}
}

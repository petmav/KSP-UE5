using UnityEngine;

[AddComponentMenu("PQuadSphere/Mods/Vertex/Color/Solid")]
public class PQSMod_VertexColorSolidBlend : PQSMod
{
	public Color color;

	public override void OnSetup()
	{
		requirements = PQS.ModiferRequirements.MeshColorChannel;
	}

	public override void OnVertexBuild(PQS.VertexBuildData data)
	{
		data.vertColor = color;
	}
}

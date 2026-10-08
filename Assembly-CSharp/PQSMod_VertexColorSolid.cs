using UnityEngine;

[AddComponentMenu("PQuadSphere/Mods/Vertex/Color/Solid (Blend)")]
public class PQSMod_VertexColorSolid : PQSMod
{
	public Color color;

	public float blend;

	private void Reset()
	{
		blend = 1f;
	}

	public override void OnSetup()
	{
		requirements = PQS.ModiferRequirements.MeshColorChannel;
	}

	public override void OnVertexBuild(PQS.VertexBuildData data)
	{
		data.vertColor = Color.Lerp(data.vertColor, color, blend);
	}
}

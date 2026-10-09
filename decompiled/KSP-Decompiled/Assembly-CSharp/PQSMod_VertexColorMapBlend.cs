using UnityEngine;

[AddComponentMenu("PQuadSphere/Mods/Vertex/Color/Map (Blend)")]
public class PQSMod_VertexColorMapBlend : PQSMod
{
	public MapSO vertexColorMap;

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
		data.vertColor = Color.Lerp(data.vertColor, vertexColorMap.GetPixelColor(data.u, data.v), blend);
	}
}

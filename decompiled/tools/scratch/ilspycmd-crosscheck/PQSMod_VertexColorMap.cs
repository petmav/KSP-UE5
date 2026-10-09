using UnityEngine;

[AddComponentMenu("PQuadSphere/Mods/Vertex/Color/Map")]
public class PQSMod_VertexColorMap : PQSMod
{
	public MapSO vertexColorMap;

	public override void OnSetup()
	{
		requirements = PQS.ModiferRequirements.MeshColorChannel;
	}

	public override void OnVertexBuild(PQS.VertexBuildData data)
	{
		data.vertColor = vertexColorMap.GetPixelColor(data.u, data.v);
	}
}

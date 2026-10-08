using UnityEngine;

[AddComponentMenu("PQuadSphere/Mods/Vertex/Height/Offset")]
public class PQSMod_VertexHeightOffset : PQSMod
{
	public double offset;

	private void Reset()
	{
		offset = 0.0;
	}

	public override void OnVertexBuildHeight(PQS.VertexBuildData data)
	{
		data.vertHeight += offset;
	}
}

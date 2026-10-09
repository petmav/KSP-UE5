using UnityEngine;

[AddComponentMenu("PQuadSphere/Mods/Vertex/Height/Flatten Ocean")]
public class PQSMod_FlattenOcean : PQSMod
{
	public double oceanRadius;

	private double oceanRad;

	private void Reset()
	{
		oceanRadius = 1.0;
	}

	public override void OnSetup()
	{
		requirements = PQS.ModiferRequirements.MeshCustomNormals;
		oceanRad = (float)(sphere.radius + oceanRadius);
	}

	public override void OnVertexBuildHeight(PQS.VertexBuildData data)
	{
		if (data.vertHeight < oceanRad)
		{
			data.vertHeight = oceanRad;
		}
	}

	public override double GetVertexMaxHeight()
	{
		return 0.0;
	}

	public override double GetVertexMinHeight()
	{
		return 0.0;
	}
}

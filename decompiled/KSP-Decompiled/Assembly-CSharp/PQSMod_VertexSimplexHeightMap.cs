using UnityEngine;

[AddComponentMenu("PQuadSphere/Mods/Vertex/Height/Simplex (HeightMap)")]
public class PQSMod_VertexSimplexHeightMap : PQSMod
{
	public int seed;

	public double deformity;

	public double octaves;

	public double persistence;

	public double frequency;

	private double n;

	private Simplex simplex;

	public MapSO heightMap;

	public float heightStart;

	public float heightEnd;

	private double hDeltaR;

	private double h;

	private void Reset()
	{
		deformity = 10.0;
		octaves = 3.0;
		persistence = 0.5;
		frequency = 1.0;
	}

	public override void OnSetup()
	{
		requirements = PQS.ModiferRequirements.MeshCustomNormals;
		simplex = new Simplex(seed, octaves, persistence, frequency);
		hDeltaR = 1.0 / (double)(heightEnd - heightStart);
	}

	public override void OnVertexBuildHeight(PQS.VertexBuildData data)
	{
		h = heightMap.GetPixelFloat(data.u, data.v);
		if (h >= (double)heightStart && h <= (double)heightEnd)
		{
			h = (h - (double)heightStart) * hDeltaR;
			data.vertHeight += (simplex.noise(data.directionFromCenter.x, data.directionFromCenter.y, data.directionFromCenter.z) + 1.0) * 0.5 * deformity * h;
		}
	}

	public override double GetVertexMaxHeight()
	{
		return deformity;
	}

	public override double GetVertexMinHeight()
	{
		return 0.0 - deformity;
	}
}

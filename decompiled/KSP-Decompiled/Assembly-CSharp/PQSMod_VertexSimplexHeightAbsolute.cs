using UnityEngine;

[AddComponentMenu("PQuadSphere/Mods/Vertex/Height/Simplex (Absolute)")]
public class PQSMod_VertexSimplexHeightAbsolute : PQSMod
{
	public int seed;

	public double deformity;

	public double octaves;

	public double persistence;

	public double frequency;

	private double n;

	private Simplex simplex;

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
	}

	public override void OnVertexBuildHeight(PQS.VertexBuildData data)
	{
		data.vertHeight += (simplex.noise(data.directionFromCenter.x, data.directionFromCenter.y, data.directionFromCenter.z) + 1.0) * 0.5 * deformity;
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

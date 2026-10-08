using UnityEngine;

[AddComponentMenu("PQuadSphere/Mods/Vertex/Color/Simplex (Multi)")]
public class PQSMod_VertexSimplexMultiChromatic : PQSMod
{
	public float blend;

	public int redSeed;

	public double redOctaves;

	public double redPersistence;

	public double redFrequency;

	private Simplex redSimplex;

	public int blueSeed;

	public double blueOctaves;

	public double bluePersistence;

	public double blueFrequency;

	private Simplex blueSimplex;

	public int greenSeed;

	public double greenOctaves;

	public double greenPersistence;

	public double greenFrequency;

	private Simplex greenSimplex;

	public int alphaSeed;

	public double alphaOctaves;

	public double alphaPersistence;

	public double alphaFrequency;

	private Simplex alphaSimplex;

	private float n;

	private Color c;

	private void Reset()
	{
		blend = 1f;
		redSeed = 1;
		redOctaves = 4.0;
		redPersistence = 0.5;
		redFrequency = 1.0;
		blueSeed = 1;
		blueOctaves = 4.0;
		bluePersistence = 0.5;
		blueFrequency = 1.0;
		greenSeed = 1;
		greenOctaves = 4.0;
		greenPersistence = 0.5;
		greenFrequency = 1.0;
		alphaSeed = 1;
		alphaOctaves = 4.0;
		alphaPersistence = 0.5;
		alphaFrequency = 1.0;
	}

	public override void OnSetup()
	{
		requirements = PQS.ModiferRequirements.MeshColorChannel;
		redSimplex = new Simplex(redSeed, redOctaves, redPersistence, redFrequency);
		blueSimplex = new Simplex(blueSeed, blueOctaves, bluePersistence, blueFrequency);
		greenSimplex = new Simplex(greenSeed, greenOctaves, greenPersistence, greenFrequency);
		alphaSimplex = new Simplex(alphaSeed, alphaOctaves, alphaPersistence, alphaFrequency);
		c = Color.white;
	}

	public override void OnVertexBuild(PQS.VertexBuildData data)
	{
		c.r = (float)redSimplex.noiseNormalized(data.directionFromCenter);
		c.g = (float)blueSimplex.noiseNormalized(data.directionFromCenter);
		c.b = (float)greenSimplex.noiseNormalized(data.directionFromCenter);
		c.a = (float)alphaSimplex.noiseNormalized(data.directionFromCenter);
		data.vertColor = Color.Lerp(data.vertColor, c, blend);
	}
}

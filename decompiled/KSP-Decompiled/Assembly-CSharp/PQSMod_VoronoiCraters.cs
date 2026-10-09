using LibNoise;
using UnityEngine;

[AddComponentMenu("PQuadSphere/Mods/Vertex/VoronoiCraters")]
public class PQSMod_VoronoiCraters : PQSMod
{
	private Voronoi voronoi;

	public double deformation = 1000.0;

	public int voronoiSeed;

	public double voronoiDisplacement = 1.0;

	public double voronoiFrequency = 1.0;

	public AnimationCurve craterCurve;

	public AnimationCurve jitterCurve;

	private Simplex simplex;

	public int simplexSeed = 123123;

	public double simplexOctaves = 2.0;

	public double simplexPersistence = 0.5;

	public double simplexFrequency = 20.0;

	public float jitter = 0.5f;

	public float jitterHeight = 0.1f;

	public Gradient craterColourRamp;

	private float vorH;

	private float spxH;

	private float jtt;

	private float r;

	private float h;

	public float rFactor = 0.5f;

	public float rOffset = 1f;

	public float colorOpacity = 1f;

	public bool DebugColorMapping;

	public override void OnSetup()
	{
		voronoi = new Voronoi(voronoiFrequency, voronoiDisplacement, voronoiSeed, distanceEnabled: true);
		simplex = new Simplex(simplexSeed, simplexOctaves, simplexPersistence, simplexFrequency);
	}

	public override void OnVertexBuildHeight(PQS.VertexBuildData data)
	{
		vorH = (float)voronoi.GetValue(data.directionFromCenter.x, data.directionFromCenter.y, data.directionFromCenter.z);
		spxH = (float)simplex.noise(data.directionFromCenter.x, data.directionFromCenter.y, data.directionFromCenter.z);
		jtt = spxH * jitter * jitterCurve.Evaluate(vorH);
		r = vorH + jtt;
		h = craterCurve.Evaluate(r);
		data.vertHeight += ((double)h + (double)(jitterHeight * jtt * h)) * deformation;
	}

	public override void OnVertexBuild(PQS.VertexBuildData data)
	{
		r = r * rFactor + rOffset;
		if (DebugColorMapping)
		{
			data.vertColor = Color.Lerp(Color.magenta, data.vertColor, r);
		}
		else
		{
			data.vertColor = Color.Lerp(data.vertColor, craterColourRamp.Evaluate(r), (1f - r) * colorOpacity);
		}
	}
}

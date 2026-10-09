using UnityEngine;

[AddComponentMenu("PQuadSphere/Mods/LOD/Enhance Coast")]
public class PQSMod_QuadEnhanceCoast : PQSMod
{
	public double coastLessThan;

	public double oceanFactor;

	public double coastFactor;

	private double coastTest;

	private void Reset()
	{
		coastLessThan = 1.0;
		oceanFactor = 0.5;
		coastFactor = 1.5;
	}

	public override void OnSetup()
	{
		coastTest = sphere.radius + coastLessThan;
	}

	public override void OnQuadBuilt(PQ quad)
	{
		if (quad.meshVertMin < coastTest)
		{
			if (quad.meshVertMax > coastTest)
			{
				quad.subdivideThresholdFactor *= coastFactor;
			}
			else
			{
				quad.subdivideThresholdFactor *= oceanFactor;
			}
		}
	}
}

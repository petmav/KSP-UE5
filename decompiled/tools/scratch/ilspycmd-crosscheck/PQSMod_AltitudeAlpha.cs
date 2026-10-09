using UnityEngine;

[AddComponentMenu("PQuadSphere/Mods/Material/Altitude to Alpha")]
public class PQSMod_AltitudeAlpha : PQSMod
{
	public double atmosphereDepth;

	public bool invert;

	private double h;

	private void Reset()
	{
		atmosphereDepth = 10000.0;
	}

	public override void OnSetup()
	{
		requirements = PQS.ModiferRequirements.MeshColorChannel;
	}

	public override void OnVertexBuild(PQS.VertexBuildData vbData)
	{
		h = (vbData.vertHeight - sphere.radius) / atmosphereDepth;
		if (invert)
		{
			vbData.vertColor.a = (float)(1.0 - h);
		}
		else
		{
			vbData.vertColor.a = (float)h;
		}
	}
}

using System;
using UnityEngine;

[AddComponentMenu("PQuadSphere/Mods/Vertex/Height/Oblate")]
public class PQSMod_VertexHeightOblate : PQSMod
{
	public double height;

	public double pow;

	private double a;

	private void Reset()
	{
		height = 1000.0;
		pow = 4.0;
	}

	public override void OnSetup()
	{
		requirements = PQS.ModiferRequirements.UVSphereCoords;
	}

	public override void OnVertexBuildHeight(PQS.VertexBuildData data)
	{
		a = Math.Sin(Math.PI * data.v);
		a = Math.Pow(a, pow);
		data.vertHeight += a * height;
	}
}

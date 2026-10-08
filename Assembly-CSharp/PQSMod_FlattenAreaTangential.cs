using System;
using UnityEngine;

[AddComponentMenu("PQuadSphere/Mods/Terrain/Flatten Area Tangential")]
public class PQSMod_FlattenAreaTangential : PQSMod
{
	public double outerRadius;

	public double innerRadius;

	public Vector3 position;

	public double flattenTo;

	public double smoothStart;

	public double smoothEnd;

	public bool DEBUG_showColors;

	private double angleInner;

	private double angleOuter;

	private double angleQuadInclusion;

	private double angleDelta;

	private bool quadActive;

	private Vector3d posNorm;

	private double testAngle;

	private double aDelta;

	private double flattenToRadius;

	private double vHeight;

	private double ct2;

	private double ct3;

	private void Reset()
	{
		outerRadius = 1000.0;
		innerRadius = 100.0;
		position = Vector3.back;
	}

	public override void OnSetup()
	{
		requirements = PQS.ModiferRequirements.MeshColorChannel | PQS.ModiferRequirements.MeshCustomNormals;
		posNorm = position.normalized;
		angleInner = Math.Atan(innerRadius / sphere.radius);
		angleOuter = Math.Atan(outerRadius / sphere.radius);
		angleQuadInclusion = angleOuter * 2.0;
		angleDelta = angleOuter - angleInner;
		flattenToRadius = sphere.radius + flattenTo;
	}

	public override void OnQuadPreBuild(PQ quad)
	{
		testAngle = Math.Acos(Vector3d.Dot(quad.positionPlanetRelative, posNorm));
		if (testAngle < angleQuadInclusion)
		{
			quadActive = true;
		}
		else
		{
			quadActive = false;
		}
	}

	public override void OnQuadBuilt(PQ quad)
	{
		quadActive = true;
	}

	public override void OnVertexBuildHeight(PQS.VertexBuildData vbData)
	{
		if (!quadActive)
		{
			return;
		}
		if (DEBUG_showColors)
		{
			vbData.vertColor = Color.green;
		}
		testAngle = Math.Acos(Vector3d.Dot(vbData.directionFromCenter, posNorm));
		vHeight = flattenToRadius / Math.Cos(testAngle);
		if (!(testAngle < angleOuter))
		{
			return;
		}
		if (testAngle < angleInner)
		{
			vbData.vertHeight = vHeight;
			if (DEBUG_showColors)
			{
				vbData.vertColor = Color.yellow;
			}
			return;
		}
		aDelta = (testAngle - angleInner) / angleDelta;
		vbData.vertHeight = CubicHermite(vHeight, vbData.vertHeight, smoothStart, smoothEnd, aDelta);
		if (DEBUG_showColors)
		{
			vbData.vertColor = Color.Lerp(Color.blue, Color.yellow, (float)(1.0 - aDelta));
		}
	}

	public double CubicHermite(double start, double end, double startTangent, double endTangent, double t)
	{
		ct2 = t * t;
		ct3 = ct2 * t;
		return start * (2.0 * ct3 - 3.0 * ct2 + 1.0) + startTangent * (ct3 - 2.0 * ct2 + t) + end * (-2.0 * ct3 + 3.0 * ct2) + endTangent * (ct3 - ct2);
	}
}

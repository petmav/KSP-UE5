using UnityEngine;

[AddComponentMenu("PQuadSphere/Mods/Misc/Remove Quads (Map)")]
public class PQSMod_RemoveQuadMap : PQSMod
{
	public MapSO map;

	public float mapDeformity;

	public float minHeight;

	public float maxHeight;

	private bool quadVisible;

	private double mapHeight;

	private void Reset()
	{
		minHeight = 0f;
		maxHeight = 1f;
	}

	public override void OnSetup()
	{
		requirements = PQS.ModiferRequirements.VertexMapCoords;
	}

	public override void OnQuadPreBuild(PQ quad)
	{
		quadVisible = false;
	}

	public override void OnVertexBuild(PQS.VertexBuildData vbData)
	{
		if (!quadVisible)
		{
			mapHeight = map.GetPixelFloat(vbData.u, vbData.v);
			if (mapHeight >= (double)minHeight && mapHeight <= (double)maxHeight)
			{
				quadVisible = true;
			}
		}
	}

	public override void OnQuadBuilt(PQ quad)
	{
		quad.isForcedInvisible = !quadVisible;
	}
}

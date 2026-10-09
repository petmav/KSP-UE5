using UnityEngine;

[AddComponentMenu("PQuadSphere/Mods/Vertex/Height/Map")]
public class PQSMod_VertexHeightMap : PQSMod
{
	public MapSO heightMap;

	public double heightMapDeformity;

	public double heightMapOffset;

	public bool scaleDeformityByRadius;

	private double heightDeformity;

	private void Reset()
	{
		heightMapDeformity = 10.0;
		heightMapOffset = 0.0;
	}

	public override void OnSetup()
	{
		requirements = PQS.ModiferRequirements.VertexMapCoords | PQS.ModiferRequirements.MeshCustomNormals;
		if (scaleDeformityByRadius)
		{
			heightDeformity = (float)(sphere.radius * heightMapDeformity);
		}
		else
		{
			heightDeformity = heightMapDeformity;
		}
	}

	public override void OnVertexBuildHeight(PQS.VertexBuildData data)
	{
		data.vertHeight += heightMapOffset + heightMapDeformity * (double)heightMap.GetPixelFloat(data.u, data.v);
	}

	public override double GetVertexMaxHeight()
	{
		return heightMapOffset + heightDeformity;
	}

	public override double GetVertexMinHeight()
	{
		return heightMapOffset;
	}

	public static double Lerp(double v2, double v1, double dt)
	{
		return v1 * dt + v2 * (1.0 - dt);
	}
}

using UnityEngine;

[AddComponentMenu("PQuadSphere/Mods/Misc/UV planet relative position")]
public class PQSMod_UVPlanetRelativePosition : PQSMod
{
	private int i;

	private Vector3d v;

	public override void OnQuadBuilt(PQ quad)
	{
		for (i = 0; i < PQS.cacheVertCount; i++)
		{
			v = PQS.verts[i];
			PQS.cacheUVs[i].x = (float)v.x;
			PQS.cacheUVs[i].y = (float)v.y;
			PQS.cacheUV2s[i].x = (float)v.z;
			PQS.cacheUV2s[i].y = (float)(1.0 - Vector3d.Dot(v.normalized, quad.vertNormals[i]));
		}
		quad.mesh.uv = PQS.cacheUVs;
		quad.mesh.uv2 = PQS.cacheUV2s;
	}

	public override void OnQuadUpdateNormals(PQ quad)
	{
		for (i = 0; i < PQS.cacheVertCount; i++)
		{
			v = quad.verts[i] + quad.positionPlanet;
			PQS.cacheUV2s[i].x = (float)v.z;
			PQS.cacheUV2s[i].y = (float)(1.0 - Vector3d.Dot(v.normalized, quad.vertNormals[i]));
		}
		quad.mesh.uv2 = PQS.cacheUV2s;
	}
}

using UnityEngine;

[AddComponentMenu("PQuadSphere/Mods/Vertex/Surface Object Quads")]
public class PQSMod_SurfaceObjectQuads : PQSMod
{
	public int maxLevelOffset;

	private int minLevel;

	private CelestialBody cb;

	private void Reset()
	{
		maxLevelOffset = 0;
	}

	public override void OnSetup()
	{
		minLevel = sphere.maxLevel - Mathf.Abs(maxLevelOffset);
		cb = sphere.GetComponentInParent<CelestialBody>();
	}

	public override void OnQuadBuilt(PQ quad)
	{
		if (quad.subdivision >= minLevel && !quad.GetComponent<SurfaceObject>())
		{
			SurfaceObject.Create(quad.gameObject, cb, 1, KFSMUpdateMode.LATEUPDATE);
		}
	}

	public override void OnQuadDestroy(PQ quad)
	{
		SurfaceObject component = quad.gameObject.GetComponent<SurfaceObject>();
		if (component != null)
		{
			component.Terminate();
		}
	}
}

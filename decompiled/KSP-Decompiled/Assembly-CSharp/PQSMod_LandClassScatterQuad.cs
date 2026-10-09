using System;
using UnityEngine;

public class PQSMod_LandClassScatterQuad : MonoBehaviour
{
	public GameObject obj;

	public Mesh mesh;

	public MeshFilter mf;

	public MeshRenderer mr;

	public bool isVisible;

	public bool isBuilt;

	public int seed;

	public PQ quad;

	public int count;

	public PQSLandControl.LandClassScatter scatter;

	public void Setup(PQ quad, int seed, PQSLandControl.LandClassScatter scatter, int count)
	{
		this.quad = quad;
		this.seed = seed;
		this.count = count;
		this.scatter = scatter;
		base.gameObject.name = "_" + quad.gameObject.name;
		isVisible = true;
		isBuilt = false;
		if (quad.onVisible != null)
		{
			quad.onVisible = (PQ.QuadDelegate)Delegate.Combine(quad.onVisible, new PQ.QuadDelegate(OnQuadVisible));
		}
		else
		{
			quad.onVisible = OnQuadVisible;
		}
		if (quad.onUpdate != null)
		{
			quad.onUpdate = (PQ.QuadDelegate)Delegate.Combine(quad.onUpdate, new PQ.QuadDelegate(OnQuadUpdate));
		}
		else
		{
			quad.onUpdate = OnQuadUpdate;
		}
		if (quad.onInvisible != null)
		{
			quad.onInvisible = (PQ.QuadDelegate)Delegate.Combine(quad.onInvisible, new PQ.QuadDelegate(OnQuadInvisible));
		}
		else
		{
			quad.onInvisible = OnQuadInvisible;
		}
		if (quad.onDestroy != null)
		{
			quad.onDestroy = (PQ.QuadDelegate)Delegate.Combine(quad.onDestroy, new PQ.QuadDelegate(OnQuadDestroy));
		}
		else
		{
			quad.onDestroy = OnQuadDestroy;
		}
		if (quad.sphereRoot.surfaceRelativeQuads)
		{
			base.transform.localPosition = quad.positionPlanet;
			base.transform.localRotation = Quaternion.identity;
		}
		else
		{
			base.transform.localPosition = Vector3.zero;
			base.transform.localRotation = Quaternion.identity;
		}
	}

	public void Destroy()
	{
		if (isVisible)
		{
			isVisible = false;
			if (isBuilt)
			{
				obj.SetActive(value: false);
				isBuilt = false;
			}
		}
		if (quad != null)
		{
			PQ pQ = quad;
			pQ.onVisible = (PQ.QuadDelegate)Delegate.Remove(pQ.onVisible, new PQ.QuadDelegate(OnQuadVisible));
			PQ pQ2 = quad;
			pQ2.onInvisible = (PQ.QuadDelegate)Delegate.Remove(pQ2.onInvisible, new PQ.QuadDelegate(OnQuadInvisible));
			PQ pQ3 = quad;
			pQ3.onDestroy = (PQ.QuadDelegate)Delegate.Remove(pQ3.onDestroy, new PQ.QuadDelegate(OnQuadDestroy));
			PQ pQ4 = quad;
			pQ4.onUpdate = (PQ.QuadDelegate)Delegate.Remove(pQ4.onUpdate, new PQ.QuadDelegate(OnQuadUpdate));
			quad = null;
		}
	}

	public void OnQuadVisible(PQ quad)
	{
		if (!isVisible)
		{
			isVisible = true;
			if (isBuilt)
			{
				obj.SetActive(value: true);
			}
		}
	}

	public void OnQuadInvisible(PQ quad)
	{
		if (isVisible)
		{
			isVisible = false;
			if (isBuilt)
			{
				obj.SetActive(value: false);
			}
		}
	}

	public void OnQuadDestroy(PQ quad)
	{
		scatter.DestroyQuad(this);
	}

	public void OnQuadUpdate(PQ quad)
	{
		if (!isBuilt && isVisible && quad.sphereRoot.quadAllowBuild && quad.sphereRoot.targetSpeed < scatter.maxSpeed)
		{
			scatter.CreateScatterMesh(this);
			isBuilt = true;
			obj.SetActive(value: true);
			quad.onUpdate = (PQ.QuadDelegate)Delegate.Remove(quad.onUpdate, new PQ.QuadDelegate(OnQuadUpdate));
		}
	}

	private void OnDestroy()
	{
		UnityEngine.Object.DestroyImmediate(mesh);
	}
}

using System;
using UnityEngine;

public class PQSMod_MeshScatter_QuadControl : MonoBehaviour
{
	public MeshFilter mf;

	public bool isVisible;

	public bool isBuilt;

	public int seed;

	public PQ quad;

	public int count;

	public PQSMod_MeshScatter scatter;

	private static double maxTargetSpeedForBuild = 10.0;

	public void Setup(PQ quad, int seed, PQSMod_MeshScatter scatter, int count)
	{
		this.quad = quad;
		this.seed = seed;
		this.scatter = scatter;
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
	}

	public void OnQuadVisible(PQ quad)
	{
		if (!isVisible)
		{
			isVisible = true;
			if (isBuilt && mf != null)
			{
				mf.GetComponent<Renderer>().enabled = true;
			}
		}
	}

	public void OnQuadInvisible(PQ quad)
	{
		if (isVisible)
		{
			isVisible = false;
			if (isBuilt && mf != null)
			{
				mf.GetComponent<Renderer>().enabled = true;
			}
		}
	}

	public void OnQuadDestroy(PQ quad)
	{
		if (isBuilt && mf != null)
		{
			scatter.UnassignScatterMesh(mf);
		}
		quad.onVisible = (PQ.QuadDelegate)Delegate.Remove(quad.onVisible, new PQ.QuadDelegate(OnQuadVisible));
		quad.onInvisible = (PQ.QuadDelegate)Delegate.Remove(quad.onInvisible, new PQ.QuadDelegate(OnQuadInvisible));
		quad.onDestroy = (PQ.QuadDelegate)Delegate.Remove(quad.onDestroy, new PQ.QuadDelegate(OnQuadDestroy));
		quad.onUpdate = (PQ.QuadDelegate)Delegate.Remove(quad.onUpdate, new PQ.QuadDelegate(OnQuadUpdate));
		UnityEngine.Object.Destroy(this);
	}

	public void OnQuadUpdate(PQ quad)
	{
		if (isVisible && !isBuilt && quad.sphereRoot.quadAllowBuild && quad.sphereRoot.targetSpeed < maxTargetSpeedForBuild)
		{
			mf = scatter.AssignScatterMesh(quad, seed, count);
			if (mf != null)
			{
				isBuilt = true;
				quad.onUpdate = (PQ.QuadDelegate)Delegate.Remove(quad.onUpdate, new PQ.QuadDelegate(OnQuadUpdate));
			}
		}
	}
}

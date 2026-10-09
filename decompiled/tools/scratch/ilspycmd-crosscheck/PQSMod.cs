using UnityEngine;

public class PQSMod : MonoBehaviour
{
	[HideInInspector]
	public PQS sphere;

	[HideInInspector]
	public PQS.ModiferRequirements requirements;

	public bool modEnabled = true;

	public int order = 100;

	internal bool overrideQuadBuildCheck;

	[SerializeField]
	internal bool modExpansionDisabled;

	[ContextMenu("Rebuild")]
	public void RebuildSphere()
	{
		if (sphere != null)
		{
			sphere.RebuildSphere();
		}
	}

	public virtual void OnSetup()
	{
		requirements = PQS.ModiferRequirements.Default;
		if (modExpansionDisabled)
		{
			modEnabled = false;
		}
	}

	public virtual void OnPostSetup()
	{
	}

	public virtual void OnSphereReset()
	{
	}

	public virtual void OnSphereActive()
	{
	}

	public virtual void OnSphereInactive()
	{
	}

	public virtual bool OnSphereStart()
	{
		return false;
	}

	public virtual void OnSphereStarted()
	{
	}

	public virtual void OnSphereTransformUpdate()
	{
	}

	public virtual void OnPreUpdate()
	{
	}

	public virtual void OnUpdateFinished()
	{
	}

	public virtual void OnVertexBuild(PQS.VertexBuildData data)
	{
	}

	public virtual void OnVertexBuildHeight(PQS.VertexBuildData data)
	{
	}

	public virtual double GetVertexMaxHeight()
	{
		return 0.0;
	}

	public virtual double GetVertexMinHeight()
	{
		return 0.0;
	}

	public virtual void OnMeshBuild()
	{
	}

	public virtual void OnQuadCreate(PQ quad)
	{
	}

	public virtual void OnQuadDestroy(PQ quad)
	{
	}

	public virtual void OnQuadPreBuild(PQ quad)
	{
	}

	public virtual void OnQuadBuilt(PQ quad)
	{
	}

	public virtual void OnQuadUpdate(PQ quad)
	{
	}

	public virtual void OnQuadUpdateNormals(PQ quad)
	{
	}
}

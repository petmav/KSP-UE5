using System;
using UnityEngine;

internal class PQSMod_ROCScatterQuad : MonoBehaviour
{
	internal GameObject rocObj;

	internal bool isVisible;

	internal bool isBuilt;

	internal int quadSeed;

	internal PQ quad;

	internal int count;

	internal LandClassROC roc;

	private bool isSetup;

	internal void Setup(PQ quad, int quadSeed, LandClassROC roc, int count, int rocIDCounter)
	{
		this.quad = quad;
		this.quadSeed = quadSeed;
		this.count = count;
		this.roc = roc;
		base.gameObject.name = "_" + roc.rocName;
		ROC component = base.gameObject.GetComponent<ROC>();
		if (component != null)
		{
			component.rocID = quadSeed + roc.rocName.GetHashCode_Net35() + rocIDCounter;
		}
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
		isSetup = true;
	}

	internal void Destroy()
	{
		if (rocObj != null)
		{
			int childCount = rocObj.transform.childCount;
			while (childCount-- > 0)
			{
				Transform child = rocObj.transform.GetChild(childCount);
				if (child != null)
				{
					child.gameObject.DestroyGameObjectImmediate();
				}
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
		isBuilt = false;
		isVisible = false;
		isSetup = false;
	}

	private void OnQuadVisible(PQ quad)
	{
		if (!isVisible)
		{
			isVisible = true;
			if (isBuilt)
			{
				rocObj.SetActive(value: true);
			}
			if (!isSetup)
			{
				Debug.LogWarningFormat("[PQSMod_ROCScatterQuad]: {0} - Quad {1} Call to OnQuadVisible but not setup!", base.name, quad.name);
			}
		}
	}

	private void OnQuadInvisible(PQ quad)
	{
		if (isVisible)
		{
			isVisible = false;
			if (isBuilt)
			{
				rocObj.SetActive(value: false);
			}
			if (!isSetup)
			{
				Debug.LogWarningFormat("[PQSMod_ROCScatterQuad]: {0} - Quad {1} Call to OnQuadInvisible but not setup!", base.name, quad.name);
			}
		}
	}

	private void OnQuadDestroy(PQ quad)
	{
		roc.DestroyQuad(this);
	}

	private void OnQuadUpdate(PQ quad)
	{
		if (isBuilt || !isVisible || !quad.sphereRoot.quadAllowBuild || !(quad.sphereRoot.target != null) || !(quad.sphereRoot.targetSpeed < (double)GameSettings.SERENITY_ROCS_VISUAL_SPEED) || quad.subdivision < roc.minLevel)
		{
			return;
		}
		if (!isSetup)
		{
			Debug.LogWarningFormat("[PQSMod_ROCScatterQuad]: {0} - Quad {1} Call to OnQuadUpdate but not setup!", base.name, quad.name);
			return;
		}
		if (rocObj.transform.childCount <= 0)
		{
			roc.CreateScatterMesh(this);
		}
		isBuilt = true;
		rocObj.SetActive(value: true);
		quad.onUpdate = (PQ.QuadDelegate)Delegate.Remove(quad.onUpdate, new PQ.QuadDelegate(OnQuadUpdate));
	}
}

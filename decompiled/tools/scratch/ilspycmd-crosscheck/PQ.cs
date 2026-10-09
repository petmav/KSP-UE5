using System;
using UnityEngine;

public class PQ : MonoBehaviour
{
	protected enum QuadEdge
	{
		North = 0,
		South = 1,
		East = 2,
		West = 3,
		Null = -1
	}

	public enum QuadChild
	{
		SouthWest = 0,
		SouthEast = 1,
		NorthWest = 2,
		NorthEast = 3,
		Null = -1
	}

	[Flags]
	public enum EdgeState
	{
		Reset = -1,
		NoLerps = 0,
		NorthLerp = 1,
		SouthLerp = 2,
		EastLerp = 4,
		WestLerp = 8
	}

	public delegate void QuadDelegate(PQ quad);

	public int id;

	public int subdivision;

	public PQS sphereRoot;

	public PQ quadRoot;

	public PQ parent;

	public PQ north;

	public PQ south;

	public PQ east;

	public PQ west;

	public PQ[] subNodes;

	public bool isActive;

	public bool isSubdivided;

	public bool isVisible;

	public bool isForcedInvisible;

	public bool isBuilt;

	public bool isQueuedForNormalUpdate;

	public bool isQueuedOnlyForCornerNormalUpdate;

	public bool isCached;

	public bool isPendingCollapse;

	[HideInInspector]
	public Mesh mesh;

	[HideInInspector]
	public Vector3[] verts;

	[HideInInspector]
	public Vector3[] vertNormals;

	[HideInInspector]
	public Vector3[][] edgeNormals;

	public Matrix4x4 quadMatrix;

	public QuaternionD planeRotation;

	public PQS.QuadPlane plane;

	public Vector3d positionPlanetRelative;

	public Vector3d positionPlanet;

	public Vector3d positionPlanePosition;

	public Vector3d positionParentRelative;

	public double scalePlanetRelative;

	public double scalePlaneRelative;

	public QuaternionD quadRotation;

	public double quadScaleFactor;

	public Vector3d quadScale;

	public double quadArea;

	public double meshVertMin;

	public double meshVertMax;

	private double angularinterval;

	public double subdivideThresholdFactor;

	public double gcDist;

	public double gcd1;

	public double gcd2;

	public PQS.EdgeState edgeState;

	private static PQS.EdgeState newEdgeState;

	public Vector2 uvSW;

	public Vector2 uvDelta;

	private static Vector2 uvMidPoint;

	private static Vector2 uvMidS;

	private static Vector2 uvMidW;

	private static Vector2 uvDel;

	public QuadDelegate onUpdate;

	public QuadDelegate onDestroy;

	public QuadDelegate onVisible;

	public QuadDelegate onInvisible;

	public MeshRenderer meshRenderer;

	public MeshFilter meshFilter;

	public MeshCollider meshCollider;

	public int quadIndex;

	public Transform quadTransform;

	private Vector3d PrecisePosition;

	private int Corner;

	public PQ CreateParent;

	private bool outOfTime;

	private void Awake()
	{
		quadTransform = base.transform;
		subNodes = new PQ[4];
		isBuilt = false;
		isVisible = false;
		edgeState = PQS.EdgeState.Reset;
	}

	public void SetupQuad(PQ parentQuad, QuadChild parentChildPosition)
	{
		if (north == null || south == null || east == null || west == null)
		{
			Debug.LogError("SetupQuad: " + base.gameObject.name);
			Debug.Break();
			Debug.DebugBreak();
		}
		angularinterval = sphereRoot.circumference * Math.Atan(scalePlaneRelative / sphereRoot.radius);
		quadRotation = Quaternion.FromToRotation(Vector3.up, positionPlanetRelative);
		if (quadRoot != null)
		{
			quadScaleFactor = 1.0 / Math.Pow(2.0, subdivision - 1);
			quadArea = quadRoot.quadArea * quadScaleFactor;
			quadScale = Vector3d.one * quadScaleFactor;
			quadMatrix = Matrix4x4.TRS(positionPlanePosition, quadRoot.planeRotation, quadScale);
			if (sphereRoot.surfaceRelativeQuads)
			{
				quadTransform.localPosition = positionPlanet;
				quadTransform.localRotation = Quaternion.identity;
				if (subdivision == sphereRoot.maxLevel)
				{
					sphereRoot.AddPQToLocalSpaceStorage(this);
					quadTransform.parent = sphereRoot.LocalSpacePQStorage.transform;
				}
				else
				{
					quadTransform.parent = sphereRoot.transform;
				}
				PrecisePosition = quadTransform.position;
				quadTransform.localScale = Vector3.one;
			}
			else
			{
				quadTransform.localPosition = Vector3.zero;
				quadTransform.localRotation = Quaternion.identity;
				if (subdivision == sphereRoot.maxLevel)
				{
					sphereRoot.AddPQToLocalSpaceStorage(this);
					quadTransform.parent = sphereRoot.LocalSpacePQStorage.transform;
				}
				else
				{
					quadTransform.parent = sphereRoot.transform;
				}
				PrecisePosition = quadTransform.position;
			}
		}
		else
		{
			planeRotation = Quaternion.FromToRotation(Vector3.up, positionPlanePosition.normalized);
			quadScaleFactor = 2.0;
			quadArea = Math.Pow(sphereRoot.radius * 2.0, 2.0);
			quadScale = Vector3d.one * quadScaleFactor;
			quadMatrix = Matrix4x4.TRS(positionPlanePosition, planeRotation, quadScale);
			if (sphereRoot.surfaceRelativeQuads)
			{
				quadTransform.localPosition = positionPlanet;
				quadTransform.localRotation = Quaternion.identity;
				quadTransform.localScale = Vector3.one;
			}
			else
			{
				quadTransform.localPosition = Vector3.zero;
				quadTransform.localRotation = Quaternion.identity;
			}
		}
		if (parentQuad == null)
		{
			quadIndex = 0;
		}
		else if (subdivision == 1)
		{
			quadIndex = (int)(parentQuad.quadIndex + parentChildPosition);
		}
		else
		{
			quadIndex = parentQuad.quadIndex + ((int)parentChildPosition << 2 * (subdivision - 1));
		}
		UpdateTargetRelativity();
	}

	internal void FastUpdateSubQuadsPosition(Vector3d movement)
	{
		PrecisePosition += movement;
		quadTransform.position = PrecisePosition;
	}

	internal void PreciseUpdateSubQuadsPosition()
	{
		quadTransform.parent = sphereRoot.transform;
		int num = Corner % 2;
		int num2 = Corner / 2;
		positionParentRelative = quadRoot.planeRotation * new Vector3d(((double)num - 0.5) * CreateParent.scalePlaneRelative, 0.0, ((double)num2 - 0.5) * CreateParent.scalePlaneRelative);
		positionPlanePosition = CreateParent.positionPlanePosition + positionParentRelative;
		positionPlanetRelative = positionPlanePosition.normalized;
		positionPlanet = positionPlanetRelative * sphereRoot.GetSurfaceHeight(positionPlanetRelative);
		quadTransform.localPosition = positionPlanet;
		quadTransform.parent = sphereRoot.LocalSpacePQStorage.transform;
		PrecisePosition = quadTransform.position;
	}

	public void UpdateTargetRelativity()
	{
		gcd1 = Math.Acos(Vector3d.Dot(positionPlanetRelative, sphereRoot.relativeTargetPositionNormalized)) * sphereRoot.radius * 1.3;
		gcDist = gcd1 + Math.Abs(sphereRoot.targetHeight) - angularinterval;
	}

	private void UpdateVisibility()
	{
		if (!isSubdivided && gcd1 < sphereRoot.visibleRadius)
		{
			SetVisible();
			newEdgeState = GetEdgeState();
			if (newEdgeState != edgeState)
			{
				mesh.triangles = PQS.cacheIndices[(int)newEdgeState];
				edgeState = newEdgeState;
				QueueForNormalUpdate();
			}
		}
		else
		{
			SetInvisible();
		}
	}

	public void UpdateSubdivision()
	{
		UpdateTargetRelativity();
		outOfTime = Time.realtimeSinceStartup > sphereRoot.maxFrameEnd;
		isPendingCollapse = false;
		if (isSubdivided)
		{
			meshRenderer.enabled = false;
			bool flag = gcDist > sphereRoot.collapseThresholds[subdivision] * subdivideThresholdFactor;
			if (subdivision <= sphereRoot.maxLevel && (!flag || outOfTime))
			{
				if (flag)
				{
					isPendingCollapse = true;
				}
				for (int i = 0; i < 4; i++)
				{
					if ((bool)subNodes[i])
					{
						subNodes[i].UpdateSubdivision();
					}
				}
			}
			else if (!Collapse())
			{
				for (int j = 0; j < 4; j++)
				{
					if ((bool)subNodes[j])
					{
						subNodes[j].UpdateSubdivision();
					}
				}
			}
		}
		else if (subdivision >= sphereRoot.minLevel && (!(gcDist < sphereRoot.subdivisionThresholds[subdivision] * subdivideThresholdFactor) || subdivision >= sphereRoot.maxLevelAtCurrentTgtSpeed || outOfTime))
		{
			UpdateVisibility();
		}
		else
		{
			Subdivide();
		}
		if (onUpdate != null)
		{
			onUpdate(this);
		}
	}

	public void UpdateSubdivisionInit()
	{
		UpdateTargetRelativity();
		if (isSubdivided)
		{
			if (subdivision > sphereRoot.minLevel && (subdivision > sphereRoot.maxLevel || gcDist > sphereRoot.collapseThresholds[subdivision] * subdivideThresholdFactor))
			{
				Collapse();
				return;
			}
			for (int i = 0; i < 4; i++)
			{
				if ((bool)subNodes[i])
				{
					subNodes[i].UpdateSubdivisionInit();
				}
			}
		}
		else if (gcDist < sphereRoot.subdivisionThresholds[subdivision] * subdivideThresholdFactor && subdivision < sphereRoot.maxLevel)
		{
			Subdivide();
		}
		else if (onUpdate != null)
		{
			onUpdate(this);
		}
	}

	public void Build()
	{
		if (isBuilt || !isActive || !sphereRoot.quadAllowBuild || isCached)
		{
			return;
		}
		if (isSubdivided)
		{
			for (int i = 0; i < 4; i++)
			{
				subNodes[i].Build();
			}
			return;
		}
		isBuilt = sphereRoot.BuildQuad(this);
		if (isBuilt)
		{
			QueueForNormalUpdate();
		}
	}

	public bool Subdivide()
	{
		if (north.subdivision < subdivision)
		{
			return false;
		}
		if (east.subdivision < subdivision)
		{
			return false;
		}
		if (south.subdivision < subdivision)
		{
			return false;
		}
		if (west.subdivision < subdivision)
		{
			return false;
		}
		if (isSubdivided)
		{
			return true;
		}
		if (!isActive)
		{
			return false;
		}
		for (int i = 0; i < 4; i++)
		{
			if (subNodes[i] != null)
			{
				subNodes[i].isActive = true;
				Debug.Log(subNodes[i].gameObject.name);
				Debug.Break();
				continue;
			}
			PQ pQ = sphereRoot.AssignQuad(subdivision + 1);
			int num = i % 2;
			int num2 = i / 2;
			pQ.scalePlaneRelative = scalePlaneRelative * 0.5;
			pQ.scalePlanetRelative = sphereRoot.radius * pQ.scalePlaneRelative;
			if (quadRoot == null)
			{
				pQ.quadRoot = this;
			}
			else
			{
				pQ.quadRoot = quadRoot;
			}
			pQ.CreateParent = this;
			pQ.positionParentRelative = pQ.quadRoot.planeRotation * new Vector3d(((double)num - 0.5) * scalePlaneRelative, 0.0, ((double)num2 - 0.5) * scalePlaneRelative);
			pQ.positionPlanePosition = positionPlanePosition + pQ.positionParentRelative;
			pQ.positionPlanetRelative = pQ.positionPlanePosition.normalized;
			pQ.positionPlanet = pQ.positionPlanetRelative * sphereRoot.GetSurfaceHeight(pQ.positionPlanetRelative);
			pQ.plane = plane;
			pQ.sphereRoot = sphereRoot;
			pQ.subdivision = subdivision + 1;
			pQ.parent = this;
			pQ.Corner = i;
			pQ.name = base.gameObject.name + i;
			pQ.gameObject.layer = base.gameObject.layer;
			sphereRoot.QuadCreated(pQ);
			subNodes[i] = pQ;
		}
		subNodes[0].north = subNodes[2];
		subNodes[0].east = subNodes[1];
		subNodes[1].north = subNodes[3];
		subNodes[1].west = subNodes[0];
		subNodes[2].south = subNodes[0];
		subNodes[2].east = subNodes[3];
		subNodes[3].south = subNodes[1];
		subNodes[3].west = subNodes[2];
		PQ left;
		PQ right;
		if (north.subdivision == subdivision && north.isSubdivided)
		{
			north.GetEdgeQuads(this, out left, out right);
			subNodes[2].north = left;
			subNodes[3].north = right;
			left.SetNeighbour(this, subNodes[2]);
			right.SetNeighbour(this, subNodes[3]);
		}
		else
		{
			subNodes[2].north = north;
			subNodes[3].north = north;
		}
		if (south.subdivision == subdivision && south.isSubdivided)
		{
			south.GetEdgeQuads(this, out left, out right);
			subNodes[1].south = left;
			subNodes[0].south = right;
			left.SetNeighbour(this, subNodes[1]);
			right.SetNeighbour(this, subNodes[0]);
		}
		else
		{
			subNodes[1].south = south;
			subNodes[0].south = south;
		}
		if (east.subdivision == subdivision && east.isSubdivided)
		{
			east.GetEdgeQuads(this, out left, out right);
			subNodes[3].east = left;
			subNodes[1].east = right;
			left.SetNeighbour(this, subNodes[3]);
			right.SetNeighbour(this, subNodes[1]);
		}
		else
		{
			subNodes[3].east = east;
			subNodes[1].east = east;
		}
		if (west.subdivision == subdivision && west.isSubdivided)
		{
			west.GetEdgeQuads(this, out left, out right);
			subNodes[0].west = left;
			subNodes[2].west = right;
			left.SetNeighbour(this, subNodes[0]);
			right.SetNeighbour(this, subNodes[2]);
		}
		else
		{
			subNodes[0].west = west;
			subNodes[2].west = west;
		}
		if (sphereRoot.reqUVQuad)
		{
			uvDel = uvDelta * 0.5f;
			uvMidPoint.x = uvSW.x + uvDel.x;
			uvMidPoint.y = uvSW.y + uvDel.y;
			uvMidS.x = uvMidPoint.x;
			uvMidS.y = uvSW.y;
			uvMidW.x = uvSW.x;
			uvMidW.y = uvMidPoint.y;
			subNodes[0].uvSW = uvMidPoint;
			subNodes[0].uvDelta = uvDel;
			subNodes[1].uvSW = uvMidW;
			subNodes[1].uvDelta = uvDel;
			subNodes[2].uvSW = uvMidS;
			subNodes[2].uvDelta = uvDel;
			subNodes[3].uvSW = uvSW;
			subNodes[3].uvDelta = uvDel;
		}
		isSubdivided = true;
		SetInvisible();
		for (int i = 0; i < 4; i++)
		{
			if (subNodes[i].north == null || subNodes[i].south == null || subNodes[i].east == null || subNodes[i].west == null)
			{
				Debug.Log("Subdivide: " + base.gameObject.name + " " + i);
				Debug.Break();
			}
			if (!subNodes[i].isCached)
			{
				subNodes[i].SetupQuad(this, (QuadChild)i);
			}
			if (sphereRoot.quadAllowBuild)
			{
				subNodes[i].UpdateVisibility();
			}
		}
		north.QueueForNormalUpdate();
		south.QueueForNormalUpdate();
		east.QueueForNormalUpdate();
		west.QueueForNormalUpdate();
		PQ rightmostCornerPQ = GetRightmostCornerPQ(north);
		if (rightmostCornerPQ != null)
		{
			rightmostCornerPQ.QueueForCornerNormalUpdate();
		}
		rightmostCornerPQ = GetRightmostCornerPQ(west);
		if (rightmostCornerPQ != null)
		{
			rightmostCornerPQ.QueueForCornerNormalUpdate();
		}
		rightmostCornerPQ = GetRightmostCornerPQ(south);
		if (rightmostCornerPQ != null)
		{
			rightmostCornerPQ.QueueForCornerNormalUpdate();
		}
		rightmostCornerPQ = GetRightmostCornerPQ(east);
		if (rightmostCornerPQ != null)
		{
			rightmostCornerPQ.QueueForCornerNormalUpdate();
		}
		return true;
	}

	public bool Collapse()
	{
		if (sphereRoot.normalUpdateList.Contains(this))
		{
			UnqueueForNormalUpdate();
		}
		if (!isSubdivided)
		{
			return false;
		}
		if (north.subdivision > subdivision)
		{
			return false;
		}
		if (south.subdivision > subdivision)
		{
			return false;
		}
		if (east.subdivision > subdivision)
		{
			return false;
		}
		if (west.subdivision > subdivision)
		{
			return false;
		}
		for (int i = 0; i < 4; i++)
		{
			if (subdivision + 1 == sphereRoot.maxLevel)
			{
				sphereRoot.RemovePQFromLocalSpaceStorage(subNodes[i]);
			}
			if (subNodes[i].isCached)
			{
				subNodes[i].SetInvisible();
				subNodes[i].isActive = false;
				if (sphereRoot.normalUpdateList.Contains(subNodes[i]))
				{
					subNodes[i].UnqueueForNormalUpdate();
				}
			}
			else
			{
				subNodes[i].ClearAndDestroy();
				subNodes[i] = null;
			}
		}
		if (north == null)
		{
			north = parent.north;
		}
		if (south == null)
		{
			south = parent.south;
		}
		if (east == null)
		{
			east = parent.east;
		}
		if (west == null)
		{
			west = parent.west;
		}
		isSubdivided = false;
		UpdateVisibility();
		north.QueueForNormalUpdate();
		south.QueueForNormalUpdate();
		east.QueueForNormalUpdate();
		west.QueueForNormalUpdate();
		PQ rightmostCornerPQ = GetRightmostCornerPQ(north);
		if (rightmostCornerPQ != null)
		{
			rightmostCornerPQ.QueueForCornerNormalUpdate();
		}
		rightmostCornerPQ = GetRightmostCornerPQ(west);
		if (rightmostCornerPQ != null)
		{
			rightmostCornerPQ.QueueForCornerNormalUpdate();
		}
		rightmostCornerPQ = GetRightmostCornerPQ(south);
		if (rightmostCornerPQ != null)
		{
			rightmostCornerPQ.QueueForCornerNormalUpdate();
		}
		rightmostCornerPQ = GetRightmostCornerPQ(east);
		if (rightmostCornerPQ != null)
		{
			rightmostCornerPQ.QueueForCornerNormalUpdate();
		}
		return true;
	}

	public void ClearAndDestroy()
	{
		if (isSubdivided)
		{
			for (int i = 0; i < 4; i++)
			{
				if (subdivision + 1 == sphereRoot.maxLevel)
				{
					sphereRoot.RemovePQFromLocalSpaceStorage(subNodes[i]);
				}
				if (subNodes[i] != null)
				{
					subNodes[i].ClearAndDestroy();
					subNodes[i] = null;
				}
			}
		}
		if (north != null && north.subdivision == subdivision)
		{
			north.SetNeighbour(this, parent);
		}
		if (east != null && east.subdivision == subdivision)
		{
			east.SetNeighbour(this, parent);
		}
		if (south != null && south.subdivision == subdivision)
		{
			south.SetNeighbour(this, parent);
		}
		if (west != null && west.subdivision == subdivision)
		{
			west.SetNeighbour(this, parent);
		}
		if (sphereRoot.normalUpdateList.Contains(this))
		{
			UnqueueForNormalUpdate();
		}
		if (onDestroy != null)
		{
			onDestroy(this);
		}
		sphereRoot.DestroyQuad(this);
	}

	public void Destroy()
	{
		if (isSubdivided)
		{
			for (int i = 0; i < 4; i++)
			{
				if (subdivision + 1 == sphereRoot.maxLevel)
				{
					sphereRoot.RemovePQFromLocalSpaceStorage(subNodes[i]);
				}
				if (subNodes[i] != null)
				{
					subNodes[i].Destroy();
					subNodes[i] = null;
				}
			}
		}
		if (sphereRoot.normalUpdateList.Contains(this))
		{
			UnqueueForNormalUpdate();
		}
		if (onDestroy != null)
		{
			onDestroy(this);
		}
		sphereRoot.DestroyQuad(this);
	}

	public void SetCache()
	{
		isCached = true;
		if (isSubdivided)
		{
			for (int i = 0; i < 4; i++)
			{
				subNodes[i].SetCache();
			}
		}
	}

	public void ClearCache()
	{
		isCached = false;
		if (isSubdivided)
		{
			for (int i = 0; i < 4; i++)
			{
				subNodes[i].ClearCache();
			}
		}
	}

	public void SetVisible()
	{
		if (!isVisible)
		{
			isVisible = true;
			if (!isBuilt)
			{
				Build();
			}
			if (isForcedInvisible)
			{
				meshRenderer.enabled = false;
			}
			else
			{
				meshRenderer.enabled = true;
			}
			if (onVisible != null)
			{
				onVisible(this);
			}
		}
	}

	public void SetInvisible()
	{
		if (isVisible)
		{
			isVisible = false;
			meshRenderer.enabled = false;
			if (onInvisible != null)
			{
				onInvisible(this);
			}
		}
	}

	public void SetMasterVisible()
	{
		if (isVisible && !isForcedInvisible)
		{
			meshRenderer.enabled = true;
		}
		if (isSubdivided)
		{
			for (int num = 3; num >= 0; num--)
			{
				subNodes[num].SetMasterVisible();
			}
		}
	}

	public void SetMasterInvisible()
	{
		if (isVisible)
		{
			meshRenderer.enabled = false;
		}
		if (isSubdivided)
		{
			for (int num = 3; num >= 0; num--)
			{
				subNodes[num].SetMasterInvisible();
			}
		}
	}

	[ContextMenu("QueueForNormalUpdate")]
	public void QueueForNormalUpdate()
	{
		if (sphereRoot != null && sphereRoot.reqCustomNormals && !isQueuedForNormalUpdate && sphereRoot.quadAllowBuild)
		{
			sphereRoot.normalUpdateList.Add(this);
			isQueuedForNormalUpdate = true;
		}
	}

	public void QueueForCornerNormalUpdate()
	{
		if (sphereRoot != null && sphereRoot.reqCustomNormals && !isQueuedForNormalUpdate && sphereRoot.quadAllowBuild)
		{
			sphereRoot.normalUpdateList.Add(this);
			isQueuedForNormalUpdate = true;
			isQueuedOnlyForCornerNormalUpdate = true;
		}
	}

	public void UnqueueForNormalUpdate()
	{
		if (sphereRoot != null && isQueuedForNormalUpdate)
		{
			sphereRoot.normalUpdateList.Remove(this);
			isQueuedForNormalUpdate = false;
			isQueuedOnlyForCornerNormalUpdate = false;
		}
	}

	public PQS.EdgeState GetEdgeState()
	{
		PQS.EdgeState edgeState = PQS.EdgeState.NoLerps;
		if (north.subdivision < subdivision)
		{
			edgeState |= PQS.EdgeState.NorthLerp;
		}
		if (south.subdivision < subdivision)
		{
			edgeState |= PQS.EdgeState.SouthLerp;
		}
		if (east.subdivision < subdivision)
		{
			edgeState |= PQS.EdgeState.EastLerp;
		}
		if (west.subdivision < subdivision)
		{
			edgeState |= PQS.EdgeState.WestLerp;
		}
		return edgeState;
	}

	public void SetEdgeState()
	{
		edgeState = PQS.EdgeState.NoLerps;
		if (north.subdivision < subdivision)
		{
			edgeState |= PQS.EdgeState.NorthLerp;
		}
		if (south.subdivision < subdivision)
		{
			edgeState |= PQS.EdgeState.SouthLerp;
		}
		if (east.subdivision < subdivision)
		{
			edgeState |= PQS.EdgeState.EastLerp;
		}
		if (west.subdivision < subdivision)
		{
			edgeState |= PQS.EdgeState.WestLerp;
		}
	}

	public void GetEdgeQuads(PQ caller, out PQ left, out PQ right)
	{
		left = null;
		right = null;
		if (north == caller)
		{
			left = subNodes[3];
			right = subNodes[2];
		}
		else if (south == caller)
		{
			left = subNodes[0];
			right = subNodes[1];
		}
		else if (east == caller)
		{
			left = subNodes[1];
			right = subNodes[3];
		}
		else if (west == caller)
		{
			left = subNodes[2];
			right = subNodes[0];
		}
		else if (north == caller.parent)
		{
			left = subNodes[3];
			right = subNodes[2];
		}
		else if (south == caller.parent)
		{
			left = subNodes[0];
			right = subNodes[1];
		}
		else if (east == caller.parent)
		{
			left = subNodes[1];
			right = subNodes[3];
		}
		else if (west == caller.parent)
		{
			left = subNodes[2];
			right = subNodes[0];
		}
	}

	public PQS.QuadEdge GetEdge(PQ caller)
	{
		if (caller == north)
		{
			return PQS.QuadEdge.North;
		}
		if (caller == south)
		{
			return PQS.QuadEdge.South;
		}
		if (caller == east)
		{
			return PQS.QuadEdge.East;
		}
		if (caller == west)
		{
			return PQS.QuadEdge.West;
		}
		return PQS.QuadEdge.Null;
	}

	public PQ GetRightmostCornerPQ(PQ nextQuad)
	{
		PQ pQ = this;
		if (nextQuad.subdivision < subdivision)
		{
			pQ = pQ.parent;
		}
		PQS.QuadEdge edge = nextQuad.GetEdge(pQ);
		if (edge == PQS.QuadEdge.Null)
		{
			if (!pQ.isPendingCollapse && !pQ.parent.isPendingCollapse && !nextQuad.isPendingCollapse && !nextQuad.parent.isPendingCollapse)
			{
				Debug.LogFormat(string.Concat("[PQ] Edge in GetRightmostCornerPQ is null! Caller: ", pQ, " nextQuad: ", nextQuad));
			}
			return null;
		}
		PQS.QuadEdge edgeRotatedCounterclockwise = PQS.GetEdgeRotatedCounterclockwise(edge);
		PQ pQ2 = nextQuad.GetSidePQ(edgeRotatedCounterclockwise);
		if (!pQ2.isBuilt)
		{
			pQ2.Build();
		}
		if (pQ2.isSubdivided)
		{
			PQ right = null;
			pQ2.GetEdgeQuads(nextQuad, out var _, out right);
			pQ2 = right;
		}
		if (!pQ2.isBuilt)
		{
			pQ2.Build();
		}
		return pQ2;
	}

	public PQ GetSidePQ(PQS.QuadEdge edge)
	{
		switch (edge)
		{
		case PQS.QuadEdge.North:
			return north;
		case PQS.QuadEdge.East:
			return east;
		case PQS.QuadEdge.South:
			return south;
		default:
			return west;
		}
	}

	public void SetNeighbour(PQ oldNeighbour, PQ newNeighbour)
	{
		if (newNeighbour == this)
		{
			return;
		}
		if (oldNeighbour == north)
		{
			north = newNeighbour;
		}
		else if (oldNeighbour == south)
		{
			south = newNeighbour;
		}
		else if (oldNeighbour == east)
		{
			east = newNeighbour;
		}
		else if (oldNeighbour == west)
		{
			west = newNeighbour;
		}
		else if (parent != null)
		{
			if (parent.north == oldNeighbour)
			{
				north = newNeighbour;
			}
			else if (parent.south == oldNeighbour)
			{
				south = newNeighbour;
			}
			else if (parent.east == oldNeighbour)
			{
				east = newNeighbour;
			}
			else if (parent.west == oldNeighbour)
			{
				west = newNeighbour;
			}
		}
	}
}

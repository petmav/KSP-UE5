using System;
using UnityEngine;

public abstract class PSpaceObject : MonoBehaviour
{
	[SerializeField]
	protected GameObject visualObject;

	[SerializeField]
	internal Mesh visualMesh;

	[SerializeField]
	protected bool procedural = true;

	protected Renderer visualRenderer;

	[SerializeField]
	internal GameObject colliderObject;

	[SerializeField]
	protected Mesh colliderMesh;

	protected SpaceObjectCollider soc;

	protected Part partComponent;

	[SerializeField]
	protected GameObject convexObject;

	[SerializeField]
	protected Mesh convexColliderMesh;

	[SerializeField]
	protected int genTime;

	public float volume;

	public float highestPoint;

	[SerializeField]
	protected float maxRange = 2500f;

	[SerializeField]
	protected float minRange;

	protected Callback onGenComplete;

	public Renderer VisualRenderer
	{
		get
		{
			if (visualRenderer == null && visualObject != null)
			{
				visualObject.GetComponentCached(ref visualRenderer);
			}
			return visualRenderer;
		}
	}

	public Mesh ConvexColliderMesh => convexColliderMesh;

	public void Awake()
	{
		if (genTime != 0)
		{
			visualMesh = null;
			colliderMesh = null;
			convexColliderMesh = null;
		}
	}

	public void Setup(Mesh visualMesh, Material visualMaterial, string visualLayer, string visualTag, Mesh colliderMesh, PhysicMaterial colliderMaterial, string colliderLayer, string colliderTag, Mesh convexMesh, PhysicMaterial convexMaterial, string convexLayer, string convexTag, Func<Transform, float> rangefinder, Callback onGenComplete)
	{
		this.visualMesh = visualMesh;
		this.colliderMesh = colliderMesh;
		convexColliderMesh = convexMesh;
		this.onGenComplete = onGenComplete;
		CreateVisual(visualMesh, visualMaterial, visualLayer, visualTag);
		CreateCollider(convexMesh, colliderMaterial, colliderLayer, colliderTag, rangefinder);
		genTime = Time.frameCount;
	}

	private void CreateVisual(Mesh visualMesh, Material visualMaterial, string visualLayer, string visualTag)
	{
		if (!(visualMesh == null))
		{
			visualObject = new GameObject("Visual");
			visualObject.transform.NestToParent(base.transform);
			visualObject.layer = LayerMask.NameToLayer(visualLayer);
			visualObject.tag = visualTag;
			visualObject.AddComponent<MeshFilter>().sharedMesh = visualMesh;
			visualObject.AddComponent<MeshRenderer>().sharedMaterial = visualMaterial;
		}
	}

	private void CreateCollider(Mesh colliderMesh, PhysicMaterial colliderMaterial, string colliderLayer, string colliderTag, Func<Transform, float> rangefinder)
	{
		if (!(colliderMesh == null))
		{
			colliderObject = new GameObject("Collider");
			colliderObject.transform.NestToParent(base.transform);
			colliderObject.layer = LayerMask.NameToLayer(colliderLayer);
			colliderObject.tag = colliderTag;
			soc = colliderObject.AddComponent<SpaceObjectCollider>();
			soc.Setup(this, colliderMesh, Vector3.zero, rangefinder, maxRange, minRange, onSOCCSetupComplete);
		}
	}

	public abstract void SetupPartParameters();

	private void CreateConvexCollider(Mesh convexMesh, PhysicMaterial convexMaterial, string convexLayer, string convexTag)
	{
		if (!(convexMesh == null))
		{
			convexObject = new GameObject("ColliderConvex");
			convexObject.transform.NestToParent(base.transform);
			convexObject.layer = LayerMask.NameToLayer(convexLayer);
			convexObject.tag = convexTag;
			MeshCollider meshCollider = convexObject.AddComponent<MeshCollider>();
			meshCollider.sharedMesh = convexMesh;
			meshCollider.sharedMaterial = convexMaterial;
			meshCollider.convex = true;
		}
	}

	private void onSOCCSetupComplete()
	{
		if (convexObject != null)
		{
			convexObject.SetActive(value: false);
		}
		onGenComplete();
	}

	private void OnDestroy()
	{
		if (procedural)
		{
			if (visualMesh != null)
			{
				UnityEngine.Object.Destroy(visualMesh);
			}
			if (colliderMesh != null)
			{
				UnityEngine.Object.Destroy(colliderMesh);
			}
			if (convexColliderMesh != null)
			{
				UnityEngine.Object.Destroy(convexColliderMesh);
			}
		}
	}

	public void SetMaterialColor(string name, Color value)
	{
		if (!(visualObject == null))
		{
			Renderer componentCached = visualObject.GetComponentCached(ref visualRenderer);
			if (componentCached != null)
			{
				componentCached.SetupProperties(new ColorMaterialProperty(name, value));
			}
		}
	}

	internal Renderer GetVisualObjectRenderer()
	{
		visualObject.GetComponentCached(ref visualRenderer);
		return visualRenderer;
	}
}

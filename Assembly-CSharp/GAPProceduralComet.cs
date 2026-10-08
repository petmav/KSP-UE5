using UnityEngine;

public class GAPProceduralComet : MonoBehaviour
{
	private string resourceURL;

	private uint seed;

	private float radius;

	private ProceduralComet pcPrefab;

	private UntrackedObjectClass sizeClass;

	public void Setup(uint seed, UntrackedObjectClass sizeClass)
	{
		this.seed = seed;
		this.sizeClass = sizeClass;
	}

	private void Start()
	{
		resourceURL = "Procedural/PC_A";
		pcPrefab = Resources.Load<ProceduralComet>(resourceURL);
		float num = (float)(sizeClass + 2) / 5f;
		radius = pcPrefab.radius * num;
		pcPrefab.Generate((int)seed, radius, base.transform, RangefinderGeneric, delegate
		{
		}, optimizeCollider: true);
	}

	private float RangefinderGeneric(Transform t)
	{
		return 0f;
	}
}

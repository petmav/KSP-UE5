using UnityEngine;

public class GAPProceduralAsteroid : MonoBehaviour
{
	private string resourceURL;

	private int seed;

	private float radius;

	private ProceduralAsteroid paPrefab;

	private UntrackedObjectClass sizeClass;

	private bool isGlimmeroid;

	public void Setup(int seed, bool isGlimmeroid, UntrackedObjectClass sizeClass)
	{
		this.seed = seed;
		this.isGlimmeroid = isGlimmeroid;
		this.sizeClass = sizeClass;
	}

	private void Start()
	{
		resourceURL = "Procedural/PA_A";
		paPrefab = Resources.Load<ProceduralAsteroid>(resourceURL);
		float num = (float)(sizeClass + 2) / 5f;
		radius = paPrefab.radius * Random.Range(num, num);
		paPrefab.Generate(seed, radius, base.transform, RangefinderGeneric, delegate
		{
		}, isGlimmeroid);
	}

	private float RangefinderGeneric(Transform t)
	{
		return 0f;
	}
}

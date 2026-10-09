using UnityEngine;

public class ProceduralAsteroidTest : MonoBehaviour
{
	[SerializeField]
	private string resourceURL;

	[SerializeField]
	private int seed;

	[SerializeField]
	private float radius;

	[SerializeField]
	private float density;

	private ProceduralAsteroid paPrefab;

	private PAsteroid paGenerated;

	[SerializeField]
	private UntrackedObjectClass objectSize;

	[SerializeField]
	public float minRadiusMultiplier = 0.75f;

	[SerializeField]
	public float maxRadiusMultiplier = 1.25f;

	private void Start()
	{
		resourceURL = resourceURL + "_" + objectSize;
		paPrefab = Resources.Load<ProceduralAsteroid>(resourceURL);
		if (paPrefab == null)
		{
			Debug.Log("Cannot find PA at URL '" + resourceURL + "'");
			return;
		}
		radius = paPrefab.radius * Random.Range(minRadiusMultiplier, maxRadiusMultiplier);
		paGenerated = paPrefab.Generate(seed, radius, base.transform, RangefinderGeneric, delegate
		{
		});
	}

	[ContextMenu("Randomize")]
	private void Randomize()
	{
		NewSeed();
		Rebuild();
	}

	[ContextMenu("Rebuild")]
	private void Rebuild()
	{
		if (!(paPrefab == null) && !(paGenerated == null) && Application.isPlaying)
		{
			Object.Destroy(paGenerated.gameObject);
			if ((bool)GetComponent<Rigidbody>())
			{
				Object.Destroy(GetComponent<Rigidbody>());
			}
			radius = paPrefab.radius * Random.Range(minRadiusMultiplier, maxRadiusMultiplier);
			paGenerated = paPrefab.Generate(seed, radius, base.transform, RangefinderGeneric, delegate
			{
			});
		}
	}

	private float RangefinderGeneric(Transform t)
	{
		return t.position.magnitude;
	}

	[ContextMenu("Add Rigidbody")]
	private void AddRigidbody()
	{
		if (!(paPrefab == null) && !(paGenerated == null) && Application.isPlaying)
		{
			base.gameObject.AddComponent<Rigidbody>().mass = density * paGenerated.volume;
		}
	}

	[ContextMenu("New Seed")]
	private void NewSeed()
	{
		seed = Random.Range(int.MinValue, int.MaxValue);
	}
}

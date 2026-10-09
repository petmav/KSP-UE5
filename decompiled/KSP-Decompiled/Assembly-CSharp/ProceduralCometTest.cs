using UnityEngine;

public class ProceduralCometTest : MonoBehaviour
{
	[SerializeField]
	private string resourceURL;

	[SerializeField]
	private int seed;

	[SerializeField]
	private float radius;

	[SerializeField]
	private float density;

	private ProceduralComet pcPrefab;

	private PComet pcGenerated;

	[SerializeField]
	private UntrackedObjectClass objectSize;

	[SerializeField]
	public float minRadiusMultiplier = 0.75f;

	[SerializeField]
	public float maxRadiusMultiplier = 1.25f;

	private void Start()
	{
		if (resourceURL.Contains("Procedural"))
		{
			resourceURL = resourceURL + "_" + objectSize;
			pcPrefab = Resources.Load<ProceduralComet>(resourceURL);
			if (pcPrefab == null)
			{
				Debug.Log("Cannot find PC at URL '" + resourceURL + "'");
				return;
			}
			radius = pcPrefab.radius * Random.Range(minRadiusMultiplier, maxRadiusMultiplier);
			pcGenerated = pcPrefab.Generate(seed, radius, base.transform, RangefinderGeneric, delegate
			{
			}, optimizeCollider: false);
		}
		else
		{
			GameObject component = Object.Instantiate(Resources.Load<PComet>(resourceURL)).GetComponent<GameObject>();
			component.name = "Comet";
			component.transform.SetParent(base.transform);
			component.transform.localPosition = Vector3.zero;
			component.transform.localRotation = Quaternion.identity;
		}
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
		if (!(pcPrefab == null) && !(pcGenerated == null) && Application.isPlaying)
		{
			Object.Destroy(pcGenerated.gameObject);
			if ((bool)GetComponent<Rigidbody>())
			{
				Object.Destroy(GetComponent<Rigidbody>());
			}
			radius = pcPrefab.radius * Random.Range(minRadiusMultiplier, maxRadiusMultiplier);
			pcGenerated = pcPrefab.Generate(seed, radius, base.transform, RangefinderGeneric, delegate
			{
			}, optimizeCollider: false);
		}
	}

	private float RangefinderGeneric(Transform t)
	{
		return t.position.magnitude;
	}

	[ContextMenu("Add Rigidbody")]
	private void AddRigidbody()
	{
		if (!(pcPrefab == null) && !(pcGenerated == null) && Application.isPlaying)
		{
			base.gameObject.AddComponent<Rigidbody>().mass = density * pcGenerated.volume;
		}
	}

	[ContextMenu("New Seed")]
	private void NewSeed()
	{
		seed = Random.Range(int.MinValue, int.MaxValue);
	}
}

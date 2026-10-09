using UnityEngine;

public class KerbalPreview : MonoBehaviour
{
	[SerializeField]
	private SkinnedMeshRenderer bodyMesh;

	[SerializeField]
	private SkinnedMeshRenderer helmetMesh;

	[SerializeField]
	private SkinnedMeshRenderer neckringMesh;

	[SerializeField]
	private GameObject helmet;

	public Material bodyMaterial;

	public Material helmetMaterial;

	public Material neckringMaterial;

	private void Awake()
	{
		bodyMaterial = bodyMesh.material;
		helmetMaterial = helmetMesh.material;
		if (neckringMesh != null)
		{
			neckringMaterial = neckringMesh.material;
		}
	}

	public void PreviewHelmetSelection(ProtoCrewMember crew)
	{
		bool hasHelmetOn;
		if (hasHelmetOn = crew.hasHelmetOn)
		{
			if (!hasHelmetOn)
			{
				return;
			}
			if (hasHelmetOn = crew.hasNeckRingOn)
			{
				if (hasHelmetOn)
				{
					helmet.SetActive(value: true);
					if (neckringMesh != null)
					{
						neckringMesh.gameObject.SetActive(value: true);
					}
				}
			}
			else
			{
				helmet.SetActive(value: true);
				if (neckringMesh != null)
				{
					neckringMesh.gameObject.SetActive(value: false);
				}
			}
		}
		else if (hasHelmetOn = crew.hasNeckRingOn)
		{
			if (hasHelmetOn)
			{
				helmet.SetActive(value: false);
				if (neckringMesh != null)
				{
					neckringMesh.gameObject.SetActive(value: true);
				}
			}
		}
		else
		{
			helmet.SetActive(value: false);
			if (neckringMesh != null)
			{
				neckringMesh.gameObject.SetActive(value: false);
			}
		}
	}
}

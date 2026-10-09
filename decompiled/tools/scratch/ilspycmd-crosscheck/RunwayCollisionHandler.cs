using System;
using UnityEngine;

public class RunwayCollisionHandler : MonoBehaviour
{
	[Serializable]
	public class RunwaySection
	{
		public DestructibleBuilding dBuilding;

		public Collider sectionCollider;

		public float sStart;

		public float sEnd;
	}

	public Vector3 runwayAxis = Vector3.forward;

	public RunwaySection[] runwaySections;

	public Collider mainCollider;

	private int loaded;

	private void Awake()
	{
		GameEvents.OnKSCStructureCollapsing.Add(onStructureCollapsing);
		GameEvents.OnKSCStructureRepaired.Add(onStructureRepaired);
		GameEvents.onGameSceneLoadRequested.Add(OnSceneLoadRequested);
	}

	private void OnDestroy()
	{
		GameEvents.OnKSCStructureCollapsing.Remove(onStructureCollapsing);
		GameEvents.OnKSCStructureRepaired.Remove(onStructureRepaired);
		GameEvents.onGameSceneLoadRequested.Remove(OnSceneLoadRequested);
		DestructibleBuilding.OnLoaded.Add(OnSectionLoaded);
	}

	private void Start()
	{
		DestructibleBuilding.OnLoaded.Remove(OnSectionLoaded);
	}

	private void OnSceneLoadRequested(GameScenes scn)
	{
		Reset();
	}

	private void OnSectionLoaded(DestructibleBuilding db)
	{
		int num = runwaySections.Length;
		while (num-- > 0)
		{
			if (runwaySections[num].dBuilding == db)
			{
				loaded++;
				break;
			}
		}
		if (loaded >= runwaySections.Length)
		{
			OnAllSectionsLoaded();
		}
	}

	private void OnAllSectionsLoaded()
	{
		int num = runwaySections.Length;
		do
		{
			if (num-- <= 0)
			{
				ReEnable();
				return;
			}
		}
		while (runwaySections[num].dBuilding.IsIntact);
		Disable();
	}

	private void OnCollisionEnter(Collision c)
	{
		if (c.contacts.Length == 0)
		{
			return;
		}
		float num = Vector3.Dot(mainCollider.transform.InverseTransformPoint(c.contacts[0].point), runwayAxis);
		int num2 = runwaySections.Length;
		RunwaySection runwaySection;
		do
		{
			if (num2-- > 0)
			{
				runwaySection = runwaySections[num2];
				continue;
			}
			return;
		}
		while (!(num > runwaySection.sStart) || num >= runwaySection.sEnd);
		runwaySection.dBuilding.OnCollisionEnter(c);
	}

	private void onStructureCollapsing(DestructibleBuilding dB)
	{
		int num = runwaySections.Length;
		do
		{
			if (num-- <= 0)
			{
				return;
			}
		}
		while (!(runwaySections[num].dBuilding == dB));
		Disable();
	}

	private void onStructureRepaired(DestructibleBuilding dB)
	{
		bool flag = false;
		int num = runwaySections.Length;
		while (true)
		{
			if (num-- > 0)
			{
				RunwaySection runwaySection = runwaySections[num];
				if (runwaySection.dBuilding.IsIntact)
				{
					if (runwaySection.dBuilding == dB)
					{
						flag = true;
					}
					continue;
				}
				break;
			}
			if (flag)
			{
				ReEnable();
			}
			break;
		}
	}

	private void Disable()
	{
		base.gameObject.SetActive(value: false);
		int num = runwaySections.Length;
		while (num-- > 0)
		{
			runwaySections[num].sectionCollider.enabled = true;
		}
	}

	private void ReEnable()
	{
		base.gameObject.SetActive(value: true);
		int num = runwaySections.Length;
		while (num-- > 0)
		{
			runwaySections[num].sectionCollider.enabled = false;
		}
	}

	private void Reset()
	{
		base.gameObject.SetActive(value: true);
		int num = runwaySections.Length;
		while (num-- > 0)
		{
			runwaySections[num].sectionCollider.enabled = false;
		}
		loaded = 0;
	}
}

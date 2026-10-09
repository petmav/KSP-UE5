using System.Collections.Generic;
using UnityEngine;

namespace KSP.UI.Screens.DebugToolbar.Screens.GamePlay
{
	public class KerbalScreen : MonoBehaviour
	{
		public KerbalScreenItem itemPrefab;

		public RectTransform listParent;

		protected bool rosterDirty;

		protected List<KerbalScreenItem> items = new List<KerbalScreenItem>();

		private void Awake()
		{
		}

		private void Start()
		{
			GameEvents.onGameStateCreated.Add(OnGameStateCreated);
			GameEvents.onGameStatePostLoad.Add(OnGameStateLoaded);
			GameEvents.onKerbalAdded.Add(OnKerbalAdded);
			GameEvents.onKerbalRemoved.Add(OnKerbalRemoved);
			GameEvents.onKerbalStatusChange.Add(OnKerbalStatusChange);
			UpdateRoster();
		}

		private void OnDestroy()
		{
			GameEvents.onGameStateCreated.Remove(OnGameStateCreated);
			GameEvents.onGameStatePostLoad.Remove(OnGameStateLoaded);
			GameEvents.onKerbalAdded.Remove(OnKerbalAdded);
			GameEvents.onKerbalRemoved.Remove(OnKerbalRemoved);
			GameEvents.onKerbalStatusChange.Remove(OnKerbalStatusChange);
		}

		private void Update()
		{
			if (rosterDirty)
			{
				UpdateRoster();
			}
		}

		private void OnGameStateCreated(Game game)
		{
			rosterDirty = true;
		}

		private void OnGameStateLoaded(ConfigNode node)
		{
			rosterDirty = true;
		}

		private void OnKerbalAdded(ProtoCrewMember pcm)
		{
			AddKerbal(pcm);
		}

		private void OnKerbalRemoved(ProtoCrewMember pcm)
		{
			RemoveKerbal(pcm);
		}

		private void OnKerbalStatusChange(ProtoCrewMember pcm, ProtoCrewMember.RosterStatus rsOld, ProtoCrewMember.RosterStatus rsNew)
		{
			UpdateKerbal(pcm);
		}

		private void UpdateRoster()
		{
			if (!base.gameObject.activeSelf)
			{
				rosterDirty = true;
				return;
			}
			rosterDirty = false;
			ClearList();
			if (HighLogic.CurrentGame != null)
			{
				int i = 0;
				for (int count = HighLogic.CurrentGame.CrewRoster.Count; i < count; i++)
				{
					CreateItem(HighLogic.CurrentGame.CrewRoster[i]);
				}
			}
		}

		private void ClearList()
		{
			int i = 0;
			for (int count = items.Count; i < count; i++)
			{
				Object.Destroy(items[i].gameObject);
			}
			items.Clear();
		}

		private void CreateItem(ProtoCrewMember pcm)
		{
			KerbalScreenItem kerbalScreenItem = Object.Instantiate(itemPrefab);
			kerbalScreenItem.transform.SetParent(listParent, worldPositionStays: false);
			kerbalScreenItem.Setup(pcm);
			items.Add(kerbalScreenItem);
		}

		private void AddKerbal(ProtoCrewMember pcm)
		{
			if (!base.gameObject.activeSelf)
			{
				rosterDirty = true;
			}
			else
			{
				if (rosterDirty)
				{
					return;
				}
				int num = 0;
				int count = items.Count;
				KerbalScreenItem kerbalScreenItem;
				while (true)
				{
					if (num < count)
					{
						kerbalScreenItem = items[num];
						if (kerbalScreenItem.name == pcm.name)
						{
							break;
						}
						num++;
						continue;
					}
					CreateItem(pcm);
					return;
				}
				kerbalScreenItem.Setup(pcm);
			}
		}

		private void RemoveKerbal(ProtoCrewMember pcm)
		{
			if (!base.gameObject.activeSelf)
			{
				rosterDirty = true;
			}
			else
			{
				if (rosterDirty)
				{
					return;
				}
				int num = 0;
				int count = items.Count;
				KerbalScreenItem kerbalScreenItem;
				while (true)
				{
					if (num < count)
					{
						kerbalScreenItem = items[num];
						if (kerbalScreenItem.name == pcm.name)
						{
							break;
						}
						num++;
						continue;
					}
					return;
				}
				items.RemoveAt(num);
				Object.Destroy(kerbalScreenItem);
			}
		}

		private void UpdateKerbal(ProtoCrewMember pcm)
		{
			if (!base.gameObject.activeSelf)
			{
				rosterDirty = true;
			}
			else
			{
				if (rosterDirty)
				{
					return;
				}
				int num = 0;
				int count = items.Count;
				KerbalScreenItem kerbalScreenItem;
				while (true)
				{
					if (num < count)
					{
						kerbalScreenItem = items[num];
						if (kerbalScreenItem.name == pcm.name)
						{
							break;
						}
						num++;
						continue;
					}
					return;
				}
				kerbalScreenItem.Setup(HighLogic.CurrentGame.CrewRoster[num]);
			}
		}
	}
}

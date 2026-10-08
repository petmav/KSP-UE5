using System.Collections.Generic;
using KSP.UI.Screens;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

namespace KSP.UI
{
	public class UILaunchsiteController : UIHoverSlidePanel
	{
		private bool groupSet;

		[SerializeField]
		private XSelectable launchSitesSelectable;

		[SerializeField]
		private EditorLaunchPadItem subassemblyPrefab;

		private List<EditorLaunchPadItem> launchPadItems;

		[SerializeField]
		private ToggleGroup selectedToggleGroup;

		public RectTransform launchsiteGrid;

		protected new void Start()
		{
			launchPadItems = new List<EditorLaunchPadItem>();
			base.Start();
			locked = false;
			groupSet = false;
			launchSitesSelectable.onPointerEnter += launchSitesSelectable_onPointerEnter;
			launchSitesSelectable.onPointerExit += launchSitesSelectable_onPointerExit;
			setupItems();
			setSelectedItem();
			GameEvents.onEditorRestart.Add(resetItems);
		}

		private void OnDestroy()
		{
			GameEvents.onEditorRestart.Remove(resetItems);
		}

		private void OnDisable()
		{
			if (coroutine != null)
			{
				StopCoroutine(coroutine);
			}
		}

		private void OnEnable()
		{
			if (!locked && !pointOver)
			{
				coroutine = StartCoroutine(MoveToState(0.1f, newState: false));
			}
		}

		private void setupItems()
		{
			for (int i = 0; i < EditorDriver.ValidLaunchSites.Count; i++)
			{
				string text = EditorDriver.ValidLaunchSites[i];
				PSystemSetup.SpaceCenterFacility spaceCenterFacility = PSystemSetup.Instance.GetSpaceCenterFacility(text);
				if (spaceCenterFacility != null)
				{
					addlaunchPadItem(spaceCenterFacility.name, spaceCenterFacility.facilityDisplayName);
					continue;
				}
				LaunchSite launchSite = PSystemSetup.Instance.GetLaunchSite(text);
				if (launchSite != null)
				{
					addlaunchPadItem(launchSite.name, launchSite.launchSiteName);
				}
			}
		}

		private void resetItems()
		{
			for (int i = 0; i < launchPadItems.Count; i++)
			{
				selectedToggleGroup.UnregisterToggle(launchPadItems[i].toggleSetDefault);
				launchPadItems[i].gameObject.DestroyGameObject();
			}
			launchPadItems.Clear();
			setupItems();
			groupSet = false;
			setSelectedItem();
		}

		private void setSelectedItem()
		{
			if (!groupSet)
			{
				setupToggleGroup();
			}
			bool flag = false;
			for (int i = 0; i < launchPadItems.Count; i++)
			{
				if (launchPadItems[i].siteName == EditorDriver.SelectedLaunchSiteName)
				{
					launchPadItems[i].toggleSetDefault.isOn = true;
					flag = true;
					break;
				}
			}
			if (!flag && launchPadItems.Count > 0)
			{
				launchPadItems[0].toggleSetDefault.isOn = true;
			}
		}

		private void addlaunchPadItem(string siteName, string displayName)
		{
			EditorLaunchPadItem editorLaunchPadItem = Object.Instantiate(subassemblyPrefab);
			editorLaunchPadItem.transform.SetParent(launchsiteGrid.transform, worldPositionStays: false);
			editorLaunchPadItem.Create(this, siteName, displayName);
			editorLaunchPadItem.gameObject.SetActive(value: true);
			launchPadItems.Add(editorLaunchPadItem);
		}

		private void setupToggleGroup()
		{
			for (int i = 0; i < launchPadItems.Count; i++)
			{
				if (launchPadItems[i].siteName == EditorDriver.SelectedLaunchSiteName)
				{
					launchPadItems[i].toggleSetDefault.isOn = true;
				}
				else
				{
					launchPadItems[i].toggleSetDefault.isOn = false;
				}
			}
			for (int j = 0; j < launchPadItems.Count; j++)
			{
				launchPadItems[j].toggleSetDefault.group = selectedToggleGroup;
				selectedToggleGroup.RegisterToggle(launchPadItems[j].toggleSetDefault);
			}
			groupSet = true;
		}

		private void launchSitesSelectable_onPointerExit(XSelectable arg1, PointerEventData arg2)
		{
			pointOver = false;
			if (!locked && coroutine == null)
			{
				coroutine = StartCoroutine(MoveToState(0.1f, newState: false));
			}
		}

		private void launchSitesSelectable_onPointerEnter(XSelectable arg1, PointerEventData arg2)
		{
			pointOver = true;
			if (!locked && coroutine == null)
			{
				coroutine = StartCoroutine(MoveToState(0f, newState: true));
			}
		}

		public override void OnPointerEnter(PointerEventData eventData)
		{
			pointOver = true;
			if (!locked && coroutine == null)
			{
				coroutine = StartCoroutine(MoveToState(0f, newState: true));
			}
		}

		public override void OnPointerExit(PointerEventData eventData)
		{
			pointOver = false;
			if (!locked && coroutine == null)
			{
				coroutine = StartCoroutine(MoveToState(0.1f, newState: false));
			}
		}
	}
}

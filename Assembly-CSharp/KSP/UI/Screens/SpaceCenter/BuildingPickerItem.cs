using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

namespace KSP.UI.Screens.SpaceCenter
{
	public class BuildingPickerItem : MonoBehaviour, IPointerEnterHandler, IEventSystemHandler, IPointerExitHandler, IPointerClickHandler
	{
		public Text buildingName;

		private SpaceCenterBuilding building;

		public Material lineMaterial;

		public Color lineColor;

		public float lineWidth;

		public float lineCornerRadius;

		private UIWorldPointer pointer;

		[SerializeField]
		private Vector2 uiPointerAnchor = new Vector2(1f, 0.5f);

		[SerializeField]
		private ButtonSpritesMgr btnSpriteMgr;

		private bool isHovering;

		private void Awake()
		{
			GameEvents.onGameUnpause.Add(OnGameUnpause);
		}

		private void OnDestroy()
		{
			GameEvents.onGameUnpause.Remove(OnGameUnpause);
		}

		public void Setup(SpaceCenterBuilding building, ButtonSpritesMgr.ButtonSprites spriteSet)
		{
			this.building = building;
			btnSpriteMgr.SetSpriteSet(spriteSet);
			building.OnClick.Add(OnBuildingClick);
			building.OnInViewChange.Add(OnBuildingInView);
		}

		public void OnPointerEnter(PointerEventData data)
		{
			isHovering = true;
			if (building.InView)
			{
				CreatePointer();
			}
			building.HighLightBuilding(mouseOverIcon: true);
		}

		private void CreatePointer()
		{
			if (pointer == null)
			{
				pointer = UIWorldPointer.Create((RectTransform)base.transform, building.BuildingTransform, FlightCamera.fetch.mainCamera, lineMaterial);
				pointer.transform.SetParent(base.transform);
				pointer.lineColor = lineColor;
				pointer.lineWidth = lineWidth;
				pointer.chamferDistance = lineCornerRadius;
				pointer.uiSnapType = UIWorldPointer.UISnapType.Anchor;
				pointer.uiSnapAnchor = uiPointerAnchor;
			}
		}

		internal void DestroyPointer()
		{
			if (pointer != null)
			{
				Object.Destroy(pointer.gameObject);
			}
		}

		public void OnPointerExit(PointerEventData data)
		{
			isHovering = false;
			if (pointer != null)
			{
				Object.Destroy(pointer.gameObject);
			}
			building.HighLightBuilding(mouseOverIcon: false);
		}

		private void OnGameUnpause()
		{
			RectTransform rectTransform = base.transform as RectTransform;
			bool flag;
			if ((flag = RectTransformUtility.ScreenPointToWorldPointInRectangle(rectTransform, Input.mousePosition, UIMainCamera.Camera, out var worldPoint) && rectTransform.Contains(worldPoint)) && !isHovering)
			{
				OnPointerEnter(null);
			}
			else if (!flag && isHovering)
			{
				OnPointerExit(null);
			}
		}

		public void OnPointerClick(PointerEventData data)
		{
			isHovering = false;
			if (data.button == PointerEventData.InputButton.Left)
			{
				building.OnLeftClick();
			}
			else if (data.button == PointerEventData.InputButton.Right)
			{
				building.OnRightClick();
			}
		}

		private void OnBuildingClick(bool leftClick)
		{
			isHovering = false;
			DestroyPointer();
		}

		private void OnBuildingInView(bool inView)
		{
			if (!inView)
			{
				DestroyPointer();
			}
			else if (isHovering)
			{
				CreatePointer();
			}
		}
	}
}

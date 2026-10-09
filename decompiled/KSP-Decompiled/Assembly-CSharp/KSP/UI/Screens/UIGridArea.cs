using System;
using TMPro;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

namespace KSP.UI.Screens
{
	public class UIGridArea : MonoBehaviour, IScrollHandler, IEventSystemHandler
	{
		public ScrollRect scrollRect;

		public RectTransform gridBackground;

		public Button zoom_up;

		public Button zoom_down;

		public TextMeshProUGUI zoom_level;

		public float zoomMin = 0.6f;

		public float zoomMax = 1f;

		public float zoomSpeed = 0.1f;

		public Material LineMaterial;

		public Material LineMaterialGray;

		public float roundedCornerRadius = 15f;

		public int lineCornerSegments = 3;

		public float lineThickness = 2f;

		[NonSerialized]
		public float gridWidth = 3071f;

		[NonSerialized]
		public float gridHeight = 2303f;

		[NonSerialized]
		public float width;

		[NonSerialized]
		public float height;

		internal float zoom = 1f;

		protected Rect snapBounds;

		internal Callback zoomCallback;

		private float scaledWidth => gridWidth * zoom - width;

		private float scaledHeight => gridHeight * zoom - height;

		protected virtual void Awake()
		{
		}

		protected virtual void Start()
		{
			zoom_up.onClick.AddListener(InputZoomUp);
			zoom_down.onClick.AddListener(InputZoomDown);
			SetZoomButtonState();
			width = (scrollRect.transform as RectTransform).rect.width;
			height = (scrollRect.transform as RectTransform).rect.height;
			gridWidth = gridBackground.rect.width;
			gridHeight = gridBackground.rect.height;
			CalculateSnapBounds();
		}

		public void OnScroll(PointerEventData eventData)
		{
			float num = Math.Sign(eventData.scrollDelta.y);
			if (num != 0f)
			{
				Zoom(num * zoomSpeed * GameSettings.AXIS_MOUSEWHEEL.scale);
			}
		}

		public void SnapToNode(RDNode node, Vector2 offset)
		{
			Vector3 vector = node.transform.position - scrollRect.transform.position;
			gridBackground.transform.position -= vector + (Vector3)offset;
			gridBackground.transform.SetLocalPositionZ();
			Snap();
		}

		protected virtual void CalculateSnapBounds()
		{
			ref Rect reference = ref snapBounds;
			ref Rect reference2 = ref snapBounds;
			float x = 0f;
			reference2.y = 0f;
			reference.x = x;
			snapBounds.xMax = scaledWidth;
			snapBounds.yMax = 0f - scaledHeight;
		}

		protected virtual void Snap()
		{
			if (gridBackground.anchoredPosition.x < snapBounds.x)
			{
				gridBackground.anchoredPosition = new Vector2(snapBounds.x, gridBackground.anchoredPosition.y);
			}
			if (gridBackground.anchoredPosition.y > snapBounds.y)
			{
				gridBackground.anchoredPosition = new Vector2(gridBackground.anchoredPosition.x, snapBounds.y);
			}
			if (gridBackground.anchoredPosition.y < snapBounds.yMax)
			{
				gridBackground.anchoredPosition = new Vector2(gridBackground.anchoredPosition.x, snapBounds.yMax);
			}
			if (gridBackground.anchoredPosition.x > snapBounds.xMax)
			{
				gridBackground.anchoredPosition = new Vector2(snapBounds.xMax, gridBackground.anchoredPosition.y);
			}
		}

		protected virtual void Zoom(float amount)
		{
			Vector2 vector = MousePointOnGrid();
			zoom += amount;
			zoom = Math.Min(zoom, zoomMax);
			zoom = Math.Max(zoom, zoomMin);
			gridBackground.transform.localScale = Vector3.one * zoom;
			Vector2 vector2 = (MousePointOnGrid() - vector) * zoom;
			gridBackground.anchoredPosition -= vector2;
			CalculateSnapBounds();
			Snap();
			SetZoomButtonState();
			SetZoomLevelText(zoom * 100f);
			if (zoomCallback != null)
			{
				zoomCallback();
			}
		}

		protected virtual void ZoomTo(float level, bool center)
		{
			Vector2 vector = Vector3.zero;
			_ = (Vector2)Vector3.zero;
			if (center)
			{
				vector = CenterOfGrid();
			}
			zoom = level;
			zoom = Math.Min(zoom, zoomMax);
			zoom = Math.Max(zoom, zoomMin);
			gridBackground.transform.localScale = Vector3.one * zoom;
			if (center)
			{
				Vector2 vector2 = (CenterOfGrid() - vector) * zoom;
				gridBackground.anchoredPosition -= vector2;
			}
			CalculateSnapBounds();
			Snap();
			SetZoomButtonState();
			SetZoomLevelText(zoom * 100f);
			if (zoomCallback != null)
			{
				zoomCallback();
			}
		}

		protected void SetZoomButtonState()
		{
			zoom_up.interactable = zoom != zoomMax;
			zoom_down.interactable = zoom != zoomMin;
		}

		protected void SetZoomLevelText(float level)
		{
			zoom_level.text = Math.Floor(level) + "%";
		}

		protected void InputZoomUp()
		{
			ZoomTo((float)Math.Round(zoom + 0.05f, 3), center: true);
		}

		protected void InputZoomDown()
		{
			ZoomTo((float)Math.Round(zoom - 0.05f, 3), center: true);
		}

		protected virtual Vector2 MousePointOnGrid()
		{
			Vector2 vector = Input.mousePosition - new Vector3(width, 16f);
			return (gridBackground.anchoredPosition - vector) * (1f / zoom);
		}

		protected virtual Vector2 CenterOfGrid()
		{
			Vector2 vector = new Vector3(width / 2f, height / 2f, 0f) - new Vector3(width, 16f);
			return (gridBackground.anchoredPosition - vector) * (1f / zoom);
		}
	}
}

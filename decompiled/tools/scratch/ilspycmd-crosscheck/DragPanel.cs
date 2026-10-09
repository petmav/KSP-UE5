using KSP.UI;
using UnityEngine;
using UnityEngine.EventSystems;

public class DragPanel : MonoBehaviour, IPointerDownHandler, IEventSystemHandler, IDragHandler
{
	private RectTransform panelRectTransform;

	public int edgeOffset = 60;

	private void Start()
	{
		panelRectTransform = base.transform as RectTransform;
		GameEvents.OnGameSettingsApplied.Add(OnGameSettingsApplied);
	}

	private void OnDestroy()
	{
		GameEvents.OnGameSettingsApplied.Remove(OnGameSettingsApplied);
	}

	private void OnGameSettingsApplied()
	{
		if (UIMasterController.AnyCornerOffScreen(panelRectTransform))
		{
			UIMasterController.DragTooltip(panelRectTransform, Vector2.zero, Vector3.one * edgeOffset);
		}
	}

	public void OnPointerDown(PointerEventData data)
	{
		panelRectTransform.SetAsLastSibling();
	}

	public void OnDrag(PointerEventData data)
	{
		if (!(panelRectTransform == null))
		{
			UIMasterController.DragTooltip(panelRectTransform, data.delta, Vector3.one * edgeOffset);
		}
	}
}

using UnityEngine.EventSystems;

public class XHoverable : UIBehaviour, IEventSystemHandler, IPointerEnterHandler, IPointerExitHandler
{
	private bool hover;

	public bool Hover => hover;

	public event Callback<XHoverable, PointerEventData> onPointerEnter;

	public event Callback<XHoverable, PointerEventData> onPointerExit;

	protected override void Awake()
	{
		base.Awake();
		this.onPointerEnter = delegate
		{
		};
		this.onPointerExit = delegate
		{
		};
	}

	public void OnPointerEnter(PointerEventData eventData)
	{
		hover = true;
		this.onPointerEnter(this, eventData);
	}

	public void OnPointerExit(PointerEventData eventData)
	{
		hover = false;
		this.onPointerExit(this, eventData);
	}

	public void Clear()
	{
		hover = false;
	}
}

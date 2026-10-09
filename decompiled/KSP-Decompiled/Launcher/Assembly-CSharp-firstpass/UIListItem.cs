using System.Runtime.CompilerServices;
using UnityEngine;

[AddComponentMenu("EZ GUI/Controls/List Item")]
public class UIListItem : UIButton, IUIListObject, IUIObject, IEZDragDrop
{
	[HideInInspector]
	public bool activeOnlyWhenSelected = true;

	protected int m_index;

	protected bool m_selected;

	protected UIScrollList list;

	protected Vector2 colliderTL;

	protected Vector2 colliderBR;

	protected Vector3 colliderCenter;

	public bool selected
	{
		get
		{
			return m_selected;
		}
		set
		{
			m_selected = value;
			if (m_selected)
			{
				SetControlState(CONTROL_STATE.ACTIVE);
			}
			else
			{
				SetControlState(CONTROL_STATE.NORMAL);
			}
		}
	}

	public override string Text
	{
		set
		{
			base.Text = value;
			FindOuterEdges();
			if (spriteText != null && spriteText.maxWidth > 0f && list != null)
			{
				list.PositionItems();
			}
		}
	}

	public int Index
	{
		get
		{
			return m_index;
		}
		set
		{
			m_index = value;
		}
	}

	public SpriteText TextObj => spriteText;

	protected override void Awake()
	{
		base.Awake();
		if (customCollider && GetComponent<Collider>() is BoxCollider)
		{
			BoxCollider boxCollider = (BoxCollider)GetComponent<Collider>();
			colliderTL.x = boxCollider.center.x - boxCollider.size.x * 0.5f;
			colliderTL.y = boxCollider.center.y + boxCollider.size.y * 0.5f;
			colliderBR.x = boxCollider.center.x + boxCollider.size.x * 0.5f;
			colliderBR.y = boxCollider.center.y - boxCollider.size.y * 0.5f;
			colliderCenter = boxCollider.center;
		}
	}

	protected void DoNeccessaryInput(ref POINTER_INFO ptr)
	{
		switch (ptr.evt)
		{
		case POINTER_INFO.INPUT_EVENT.NO_CHANGE:
			if (list != null && ptr.active)
			{
				list.ListDragged(ptr);
			}
			break;
		case POINTER_INFO.INPUT_EVENT.DRAG:
			if (list != null && !ptr.isTap)
			{
				list.ListDragged(ptr);
			}
			break;
		case POINTER_INFO.INPUT_EVENT.RELEASE:
		case POINTER_INFO.INPUT_EVENT.TAP:
		case POINTER_INFO.INPUT_EVENT.RELEASE_OFF:
			if (list != null)
			{
				list.PointerReleased();
			}
			break;
		}
		if (list != null && ptr.inputDelta.z != 0f && ptr.type != POINTER_INFO.POINTER_TYPE.RAY)
		{
			list.ScrollWheel(ptr.inputDelta.z);
		}
		if (Container != null)
		{
			ptr.callerIsControl = true;
			Container.OnInput(ptr);
		}
	}

	public override void OnInput(ref POINTER_INFO ptr)
	{
		if (deleted)
		{
			return;
		}
		if (!m_controlIsEnabled)
		{
			DoNeccessaryInput(ref ptr);
			return;
		}
		if (list != null && Vector3.SqrMagnitude(ptr.origPos - ptr.devicePos) > list.dragThreshold * list.dragThreshold)
		{
			ptr.isTap = false;
			if (ptr.evt == POINTER_INFO.INPUT_EVENT.TAP)
			{
				ptr.evt = POINTER_INFO.INPUT_EVENT.RELEASE;
			}
		}
		else
		{
			ptr.isTap = true;
		}
		if (inputDelegate != null)
		{
			inputDelegate(ref ptr);
		}
		if (!m_controlIsEnabled)
		{
			DoNeccessaryInput(ref ptr);
			return;
		}
		switch (ptr.evt)
		{
		case POINTER_INFO.INPUT_EVENT.NO_CHANGE:
			if (list != null && ptr.active)
			{
				list.ListDragged(ptr);
			}
			break;
		case POINTER_INFO.INPUT_EVENT.MOVE:
			if (!selected)
			{
				if (soundOnOver != null && m_ctrlState != CONTROL_STATE.OVER)
				{
					soundOnOver.PlayOneShot(soundOnOver.clip);
				}
				SetControlState(CONTROL_STATE.OVER);
			}
			break;
		case POINTER_INFO.INPUT_EVENT.DRAG:
			if (!ptr.isTap)
			{
				if (!selected)
				{
					SetControlState(CONTROL_STATE.NORMAL);
				}
				if (list != null)
				{
					list.ListDragged(ptr);
				}
			}
			else if (!activeOnlyWhenSelected)
			{
				SetControlState(CONTROL_STATE.ACTIVE);
			}
			break;
		case POINTER_INFO.INPUT_EVENT.PRESS:
			if (!activeOnlyWhenSelected)
			{
				SetControlState(CONTROL_STATE.ACTIVE);
			}
			break;
		case POINTER_INFO.INPUT_EVENT.TAP:
			if (list != null)
			{
				list.DidSelect(this);
				list.PointerReleased();
			}
			break;
		case POINTER_INFO.INPUT_EVENT.RELEASE:
		case POINTER_INFO.INPUT_EVENT.RELEASE_OFF:
			if (!selected)
			{
				SetControlState(CONTROL_STATE.NORMAL);
			}
			if (list != null)
			{
				list.PointerReleased();
			}
			break;
		case POINTER_INFO.INPUT_EVENT.MOVE_OFF:
			if (!selected)
			{
				SetControlState(CONTROL_STATE.NORMAL);
			}
			break;
		}
		if (list != null && ptr.inputDelta.z != 0f && ptr.type != POINTER_INFO.POINTER_TYPE.RAY)
		{
			list.ScrollWheel(ptr.inputDelta.z);
		}
		if (Container != null)
		{
			ptr.callerIsControl = true;
			Container.OnInput(ptr);
		}
		if (repeat)
		{
			if (m_ctrlState != CONTROL_STATE.ACTIVE)
			{
				return;
			}
		}
		else if (ptr.evt != whenToInvoke)
		{
			return;
		}
		if (ptr.evt == whenToInvoke && soundOnClick != null)
		{
			soundOnClick.PlayOneShot(soundOnClick.clip);
		}
		if (scriptWithMethodToInvoke != null)
		{
			scriptWithMethodToInvoke.Invoke(methodToInvoke, delay);
		}
		if (changeDelegate != null)
		{
			changeDelegate(this);
		}
	}

	protected override void OnEnable()
	{
		base.OnEnable();
		if (!Application.isPlaying || !m_started)
		{
			return;
		}
		m_ctrlState = (CONTROL_STATE)(-1);
		if (controlIsEnabled)
		{
			if (selected)
			{
				SetControlState(CONTROL_STATE.ACTIVE);
			}
			else
			{
				SetControlState(CONTROL_STATE.NORMAL, suppressTransitions: true);
			}
		}
		else
		{
			SetControlState(CONTROL_STATE.DISABLED, suppressTransitions: true);
		}
	}

	protected override void OnDisable()
	{
		CONTROL_STATE cONTROL_STATE = base.controlState;
		base.OnDisable();
		SetControlState(cONTROL_STATE);
	}

	public override void Copy(SpriteRoot s)
	{
		Copy(s, ControlCopyFlags.All);
	}

	public override void Copy(SpriteRoot s, ControlCopyFlags flags)
	{
		base.Copy(s, flags);
		if (s is UIListItem)
		{
			UIListItem uIListItem = (UIListItem)s;
			if ((flags & ControlCopyFlags.Settings) == ControlCopyFlags.Settings)
			{
				list = uIListItem.list;
			}
			if ((flags & ControlCopyFlags.Appearance) == ControlCopyFlags.Appearance)
			{
				topLeftEdge = uIListItem.topLeftEdge;
				bottomRightEdge = uIListItem.bottomRightEdge;
				colliderTL = uIListItem.colliderTL;
				colliderBR = uIListItem.colliderBR;
				colliderCenter = uIListItem.colliderCenter;
				customCollider = uIListItem.customCollider;
			}
		}
	}

	public override void FindOuterEdges()
	{
		base.FindOuterEdges();
		if (!customCollider)
		{
			colliderTL = topLeftEdge;
			colliderBR = bottomRightEdge;
		}
	}

	public override void TruncateRight(float pct)
	{
		base.TruncateRight(pct);
		if (GetComponent<Collider>() != null && GetComponent<Collider>() is BoxCollider)
		{
			if (customCollider)
			{
				BoxCollider obj = (BoxCollider)GetComponent<Collider>();
				Vector3 center = obj.center;
				center.x = (1f - pct) * (colliderBR.x - colliderTL.x) * -0.5f;
				obj.center = center;
				center = obj.size;
				center.x = pct * (colliderBR.x - colliderTL.x);
				obj.size = center;
			}
			else
			{
				UpdateCollider();
			}
		}
	}

	public override void TruncateLeft(float pct)
	{
		base.TruncateLeft(pct);
		if (GetComponent<Collider>() != null && GetComponent<Collider>() is BoxCollider)
		{
			if (customCollider)
			{
				BoxCollider obj = (BoxCollider)GetComponent<Collider>();
				Vector3 center = obj.center;
				center.x = (1f - pct) * (colliderBR.x - colliderTL.x) * 0.5f;
				obj.center = center;
				center = obj.size;
				center.x = pct * (colliderBR.x - colliderTL.x);
				obj.size = center;
			}
			else
			{
				UpdateCollider();
			}
		}
	}

	public override void TruncateTop(float pct)
	{
		base.TruncateTop(pct);
		if (GetComponent<Collider>() != null && GetComponent<Collider>() is BoxCollider)
		{
			if (customCollider)
			{
				BoxCollider obj = (BoxCollider)GetComponent<Collider>();
				Vector3 center = obj.center;
				center.y = (1f - pct) * (colliderBR.y - colliderTL.y) * 0.5f;
				obj.center = center;
				center = obj.size;
				center.y = pct * (colliderTL.y - colliderBR.y);
				obj.size = center;
			}
			else
			{
				UpdateCollider();
			}
		}
	}

	public override void TruncateBottom(float pct)
	{
		base.TruncateBottom(pct);
		if (GetComponent<Collider>() != null && GetComponent<Collider>() is BoxCollider)
		{
			if (customCollider)
			{
				BoxCollider obj = (BoxCollider)GetComponent<Collider>();
				Vector3 center = obj.center;
				center.y = (1f - pct) * (colliderBR.y - colliderTL.y) * -0.5f;
				obj.center = center;
				center = obj.size;
				center.y = pct * (colliderTL.y - colliderBR.y);
				obj.size = center;
			}
			else
			{
				UpdateCollider();
			}
		}
	}

	public override void Untruncate()
	{
		base.Untruncate();
		if (!(GetComponent<Collider>() != null) || !(GetComponent<Collider>() is BoxCollider))
		{
			return;
		}
		if (customCollider)
		{
			BoxCollider boxCollider = (BoxCollider)GetComponent<Collider>();
			if (!customCollider)
			{
				boxCollider.center = Vector3.zero;
			}
			else
			{
				boxCollider.center = colliderCenter;
			}
			boxCollider.size = new Vector3(colliderBR.x - colliderTL.x, colliderTL.y - colliderBR.y, 0.001f);
		}
		else
		{
			UpdateCollider();
		}
	}

	public override void Hide(bool tf)
	{
		base.Hide(tf);
		for (int i = 0; i < layers.Length; i++)
		{
			layers[i].Hide(tf);
		}
		if (spriteText != null)
		{
			if (tf)
			{
				spriteText.gameObject.active = false;
			}
			else
			{
				spriteText.gameObject.active = true;
			}
		}
	}

	public void SetList(UIScrollList c)
	{
		list = c;
	}

	public virtual UIScrollList GetScrollList()
	{
		return list;
	}

	public void FindText()
	{
		if (spriteText == null)
		{
			spriteText = (SpriteText)GetComponentInChildren(typeof(SpriteText));
		}
		if (spriteText != null)
		{
			spriteText.gameObject.layer = base.gameObject.layer;
			spriteText.Parent = this;
		}
	}

	public bool IsContainer()
	{
		return false;
	}

	public new static UIListItem Create(string name, Vector3 pos)
	{
		GameObject obj = new GameObject(name);
		obj.transform.position = pos;
		return (UIListItem)obj.AddComponent(typeof(UIListItem));
	}

	public new static UIListItem Create(string name, Vector3 pos, Quaternion rotation)
	{
		GameObject obj = new GameObject(name);
		obj.transform.position = pos;
		obj.transform.rotation = rotation;
		return (UIListItem)obj.AddComponent(typeof(UIListItem));
	}

	[SpecialName]
	GameObject IUIObject.get_gameObject()
	{
		return base.gameObject;
	}

	[SpecialName]
	Transform IUIObject.get_transform()
	{
		return base.transform;
	}

	[SpecialName]
	string IUIObject.get_name()
	{
		return base.name;
	}
}

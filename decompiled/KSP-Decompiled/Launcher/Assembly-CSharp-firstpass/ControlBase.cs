using System;
using System.Runtime.CompilerServices;
using UnityEngine;

[ExecuteInEditMode]
public abstract class ControlBase : MonoBehaviour, IControl, IUIObject, IEZDragDrop
{
	protected ControlBaseMirror mirror;

	public const string DittoString = "[\"]";

	public string text;

	public SpriteText spriteText;

	public float textOffsetZ = -0.1f;

	public bool includeTextInAutoCollider;

	protected SpriteText.Anchor_Pos defaultTextAnchor = SpriteText.Anchor_Pos.Middle_Center;

	protected SpriteText.Alignment_Type defaultTextAlignment = SpriteText.Alignment_Type.Center;

	protected bool deleted;

	public bool detargetOnDisable;

	protected bool customCollider;

	[HideInInspector]
	public object data;

	protected bool m_controlIsEnabled = true;

	protected IUIContainer container;

	protected EZInputDelegate inputDelegate;

	protected EZValueChangedDelegate changeDelegate;

	protected EZDragDropHelper dragDropHelper = new EZDragDropHelper();

	public bool isDraggable;

	public LayerMask dropMask = -1;

	public float dragOffset = float.NaN;

	public EZAnimation.EASING_TYPE cancelDragEasing = EZAnimation.EASING_TYPE.Default;

	public float cancelDragDuration = -1f;

	public virtual string Text
	{
		get
		{
			return text;
		}
		set
		{
			text = value;
			if (spriteText == null)
			{
				if (text == "")
				{
					return;
				}
				if (UIManager.instance == null)
				{
					Debug.LogWarning("Warning: No UIManager exists in the scene. A UIManager with a default font is required to automatically add text to a control.");
					return;
				}
				if (UIManager.instance.defaultFont == null)
				{
					Debug.LogWarning("Warning: No default font defined.  A UIManager object with a default font is required to automatically add text to a control.");
					return;
				}
				GameObject gameObject = new GameObject();
				gameObject.layer = base.gameObject.layer;
				gameObject.transform.parent = base.transform;
				gameObject.transform.localPosition = Vector3.zero;
				gameObject.transform.localRotation = Quaternion.identity;
				gameObject.name = "control_text";
				((MeshRenderer)gameObject.AddComponent(typeof(MeshRenderer))).material = UIManager.instance.defaultFontMaterial;
				spriteText = (SpriteText)gameObject.AddComponent(typeof(SpriteText));
				spriteText.font = UIManager.instance.defaultFont;
				spriteText.offsetZ = textOffsetZ;
				spriteText.Parent = this;
				spriteText.anchor = defaultTextAnchor;
				spriteText.alignment = defaultTextAlignment;
				spriteText.pixelPerfect = true;
				spriteText.Start();
			}
			spriteText.Text = text;
			text = spriteText.Text;
			if (includeTextInAutoCollider)
			{
				UpdateCollider();
			}
		}
	}

	public object Data
	{
		get
		{
			return data;
		}
		set
		{
			data = value;
		}
	}

	public virtual bool IncludeTextInAutoCollider
	{
		get
		{
			return includeTextInAutoCollider;
		}
		set
		{
			includeTextInAutoCollider = value;
			UpdateCollider();
		}
	}

	public virtual bool controlIsEnabled
	{
		get
		{
			return m_controlIsEnabled;
		}
		set
		{
			m_controlIsEnabled = value;
		}
	}

	public virtual bool DetargetOnDisable
	{
		get
		{
			return DetargetOnDisable;
		}
		set
		{
			DetargetOnDisable = value;
		}
	}

	public virtual IUIContainer Container
	{
		get
		{
			return container;
		}
		set
		{
			if (container != null && spriteText != null)
			{
				container.RemoveChild(spriteText.gameObject);
			}
			if (value != null && spriteText != null)
			{
				value.AddChild(spriteText.gameObject);
			}
			container = value;
		}
	}

	public bool IsDraggable
	{
		get
		{
			return isDraggable;
		}
		set
		{
			isDraggable = value;
		}
	}

	public LayerMask DropMask
	{
		get
		{
			return dropMask;
		}
		set
		{
			dropMask = value;
		}
	}

	public float DragOffset
	{
		get
		{
			return dragOffset;
		}
		set
		{
			dragOffset = value;
			if (IsDragging && UIManager.Exists() && UIManager.instance.GetPointer(this, out var ptr))
			{
				dragDropHelper.DragUpdatePosition(ptr);
			}
		}
	}

	public EZAnimation.EASING_TYPE CancelDragEasing
	{
		get
		{
			return cancelDragEasing;
		}
		set
		{
			cancelDragEasing = value;
		}
	}

	public float CancelDragDuration
	{
		get
		{
			return cancelDragDuration;
		}
		set
		{
			cancelDragDuration = value;
		}
	}

	public bool IsDragging
	{
		get
		{
			return dragDropHelper.IsDragging;
		}
		set
		{
			dragDropHelper.IsDragging = value;
		}
	}

	public GameObject DropTarget
	{
		get
		{
			return dragDropHelper.DropTarget;
		}
		set
		{
			dragDropHelper.DropTarget = value;
		}
	}

	public bool DropHandled
	{
		get
		{
			return dragDropHelper.DropHandled;
		}
		set
		{
			dragDropHelper.DropHandled = value;
		}
	}

	public bool UseDefaultCancelDragAnim
	{
		get
		{
			return dragDropHelper.UseDefaultCancelDragAnim;
		}
		set
		{
			dragDropHelper.UseDefaultCancelDragAnim = value;
		}
	}

	public abstract string[] States { get; }

	public abstract EZTransitionList[] Transitions { get; set; }

	protected virtual void Awake()
	{
		if (GetComponent<Collider>() != null)
		{
			customCollider = true;
		}
		if (dragDropHelper == null)
		{
			dragDropHelper = new EZDragDropHelper(this);
		}
		else
		{
			dragDropHelper.host = this;
		}
	}

	public virtual void Start()
	{
		if (spriteText != null)
		{
			spriteText.Parent = this;
		}
		if (UIManager.Exists() && Application.isPlaying)
		{
			if (cancelDragEasing == EZAnimation.EASING_TYPE.Default)
			{
				cancelDragEasing = UIManager.instance.cancelDragEasing;
			}
			if (cancelDragDuration == -1f)
			{
				cancelDragDuration = UIManager.instance.cancelDragDuration;
			}
			if (float.IsNaN(dragOffset))
			{
				dragOffset = UIManager.instance.defaultDragOffset;
			}
		}
	}

	protected virtual void AddCollider()
	{
		if (!customCollider)
		{
			base.gameObject.AddComponent(typeof(BoxCollider));
			UpdateCollider();
		}
	}

	public virtual void UpdateCollider()
	{
		if (!customCollider && GetComponent<Collider>() is BoxCollider)
		{
			BoxCollider boxCollider = (BoxCollider)GetComponent<Collider>();
			if (includeTextInAutoCollider && spriteText != null)
			{
				Bounds bounds = new Bounds(boxCollider.center, boxCollider.size);
				Matrix4x4 localToWorldMatrix = spriteText.transform.localToWorldMatrix;
				Matrix4x4 worldToLocalMatrix = base.transform.worldToLocalMatrix;
				Vector3 point = worldToLocalMatrix.MultiplyPoint3x4(localToWorldMatrix.MultiplyPoint3x4(spriteText.TopLeft)) * 2f;
				Vector3 point2 = worldToLocalMatrix.MultiplyPoint3x4(localToWorldMatrix.MultiplyPoint3x4(spriteText.BottomRight)) * 2f;
				bounds.Encapsulate(point);
				bounds.Encapsulate(point2);
				boxCollider.size = bounds.extents;
				boxCollider.center = bounds.center * 0.5f;
			}
			boxCollider.isTrigger = true;
		}
	}

	public IUIObject GetControl(ref POINTER_INFO ptr)
	{
		return this;
	}

	public bool RequestContainership(IUIContainer cont)
	{
		Transform parent = base.transform.parent;
		Transform transform = ((Component)cont).transform;
		while (parent != null)
		{
			if (parent == transform)
			{
				Container = cont;
				return true;
			}
			if (parent.gameObject.GetComponent("IUIContainer") != null)
			{
				return false;
			}
			parent = parent.parent;
		}
		return false;
	}

	public virtual bool GotFocus()
	{
		return false;
	}

	public virtual void SetInputDelegate(EZInputDelegate del)
	{
		inputDelegate = del;
	}

	public virtual void AddInputDelegate(EZInputDelegate del)
	{
		inputDelegate = (EZInputDelegate)Delegate.Combine(inputDelegate, del);
	}

	public virtual void RemoveInputDelegate(EZInputDelegate del)
	{
		inputDelegate = (EZInputDelegate)Delegate.Remove(inputDelegate, del);
	}

	public virtual void SetValueChangedDelegate(EZValueChangedDelegate del)
	{
		changeDelegate = del;
	}

	public virtual void AddValueChangedDelegate(EZValueChangedDelegate del)
	{
		changeDelegate = (EZValueChangedDelegate)Delegate.Combine(changeDelegate, del);
	}

	public virtual void RemoveValueChangedDelegate(EZValueChangedDelegate del)
	{
		changeDelegate = (EZValueChangedDelegate)Delegate.Remove(changeDelegate, del);
	}

	public virtual void OnInput(POINTER_INFO ptr)
	{
		OnInput(ref ptr);
	}

	public virtual void OnInput(ref POINTER_INFO ptr)
	{
		if (Container != null)
		{
			ptr.callerIsControl = true;
			Container.OnInput(ptr);
		}
	}

	public virtual void OnEnable()
	{
	}

	public virtual void OnDisable()
	{
		if (Application.isPlaying)
		{
			if (EZAnimator.Exists())
			{
				EZAnimator.instance.Stop(base.gameObject);
				EZAnimator.instance.Stop(this);
			}
			if (detargetOnDisable && UIManager.Exists())
			{
				UIManager.instance.Detarget(this);
			}
		}
	}

	public virtual void OnDestroy()
	{
		deleted = true;
	}

	public virtual void Copy(IControl ctl)
	{
		Copy(ctl, ControlCopyFlags.All);
	}

	public virtual void Copy(IControl ctl, ControlCopyFlags flags)
	{
		if (!(ctl is ControlBase))
		{
			return;
		}
		ControlBase controlBase = (ControlBase)ctl;
		if ((flags & ControlCopyFlags.Transitions) == ControlCopyFlags.Transitions)
		{
			if (controlBase is UIStateToggleBtn3D)
			{
				if (controlBase.Transitions != null)
				{
					((UIStateToggleBtn3D)this).transitions = new EZTransitionList[controlBase.Transitions.Length];
					for (int i = 0; i < Transitions.Length; i++)
					{
						controlBase.Transitions[i].CopyToNew(Transitions[i], copyInit: true);
					}
				}
			}
			else if (Transitions != null && controlBase.Transitions != null)
			{
				for (int j = 0; j < Transitions.Length && j < controlBase.Transitions.Length; j++)
				{
					controlBase.Transitions[j].CopyTo(Transitions[j], copyInit: true);
				}
			}
		}
		if ((flags & ControlCopyFlags.Text) == ControlCopyFlags.Text)
		{
			if (spriteText == null && controlBase.spriteText != null)
			{
				GameObject obj = UnityEngine.Object.Instantiate(controlBase.spriteText.gameObject);
				obj.transform.parent = base.transform;
				obj.transform.localPosition = controlBase.spriteText.transform.localPosition;
				obj.transform.localScale = controlBase.spriteText.transform.localScale;
				obj.transform.localRotation = controlBase.spriteText.transform.localRotation;
			}
			Text = controlBase.Text;
		}
		if ((flags & ControlCopyFlags.Appearance) == ControlCopyFlags.Appearance && GetComponent<Collider>().GetType() == controlBase.GetComponent<Collider>().GetType())
		{
			if (GetComponent<Collider>() is BoxCollider)
			{
				BoxCollider obj2 = (BoxCollider)GetComponent<Collider>();
				BoxCollider boxCollider = (BoxCollider)controlBase.GetComponent<Collider>();
				obj2.center = boxCollider.center;
				obj2.size = boxCollider.size;
			}
			else if (GetComponent<Collider>() is SphereCollider)
			{
				SphereCollider obj3 = (SphereCollider)GetComponent<Collider>();
				SphereCollider sphereCollider = (SphereCollider)controlBase.GetComponent<Collider>();
				obj3.center = sphereCollider.center;
				obj3.radius = sphereCollider.radius;
			}
			else if (GetComponent<Collider>() is CapsuleCollider)
			{
				CapsuleCollider obj4 = (CapsuleCollider)GetComponent<Collider>();
				CapsuleCollider capsuleCollider = (CapsuleCollider)controlBase.GetComponent<Collider>();
				obj4.center = capsuleCollider.center;
				obj4.radius = capsuleCollider.radius;
				obj4.height = capsuleCollider.height;
				obj4.direction = capsuleCollider.direction;
			}
			else if (GetComponent<Collider>() is MeshCollider)
			{
				MeshCollider obj5 = (MeshCollider)GetComponent<Collider>();
				MeshCollider meshCollider = (MeshCollider)controlBase.GetComponent<Collider>();
				obj5.smoothSphereCollisions = meshCollider.smoothSphereCollisions;
				obj5.convex = meshCollider.convex;
				obj5.sharedMesh = meshCollider.sharedMesh;
			}
			GetComponent<Collider>().isTrigger = controlBase.GetComponent<Collider>().isTrigger;
		}
		if ((flags & ControlCopyFlags.Invocation) == ControlCopyFlags.Invocation)
		{
			changeDelegate = controlBase.changeDelegate;
			inputDelegate = controlBase.inputDelegate;
		}
		if ((flags & ControlCopyFlags.State) == ControlCopyFlags.State)
		{
			Container = controlBase.Container;
			if (Application.isPlaying)
			{
				controlIsEnabled = controlBase.controlIsEnabled;
			}
		}
	}

	public void DragUpdatePosition(POINTER_INFO ptr)
	{
		dragDropHelper.DragUpdatePosition(ptr);
	}

	public void DefaultDragUpdatePosition(POINTER_INFO ptr)
	{
		dragDropHelper.DefaultDragUpdatePosition(ptr);
	}

	public void SetDragPosUpdater(EZDragDropHelper.UpdateDragPositionDelegate del)
	{
		dragDropHelper.SetDragPosUpdater(del);
	}

	public void CancelDrag()
	{
		dragDropHelper.CancelDrag();
	}

	public void CancelFinished()
	{
		dragDropHelper.CancelFinished();
	}

	public void DoDefaultCancelDrag()
	{
		dragDropHelper.DoDefaultCancelDrag();
	}

	public void OnEZDragDrop_Internal(EZDragDropParams parms)
	{
		dragDropHelper.OnEZDragDrop_Internal(parms);
	}

	public void AddDragDropDelegate(EZDragDropDelegate del)
	{
		dragDropHelper.AddDragDropDelegate(del);
	}

	public void RemoveDragDropDelegate(EZDragDropDelegate del)
	{
		dragDropHelper.RemoveDragDropDelegate(del);
	}

	public void SetDragDropDelegate(EZDragDropDelegate del)
	{
		dragDropHelper.SetDragDropDelegate(del);
	}

	public void SetDragDropInternalDelegate(EZDragDropHelper.DragDrop_InternalDelegate del)
	{
		dragDropHelper.SetDragDropInternalDelegate(del);
	}

	public void AddDragDropInternalDelegate(EZDragDropHelper.DragDrop_InternalDelegate del)
	{
		dragDropHelper.AddDragDropInternalDelegate(del);
	}

	public void RemoveDragDropInternalDelegate(EZDragDropHelper.DragDrop_InternalDelegate del)
	{
		dragDropHelper.RemoveDragDropInternalDelegate(del);
	}

	public EZDragDropHelper.DragDrop_InternalDelegate GetDragDropInternalDelegate()
	{
		return dragDropHelper.GetDragDropInternalDelegate();
	}

	public virtual int DrawPreStateSelectGUI(int selState, bool inspector)
	{
		return 0;
	}

	public virtual int DrawPostStateSelectGUI(int selState)
	{
		return 0;
	}

	public virtual void DrawPreTransitionUI(int selState, IGUIScriptSelector gui)
	{
	}

	public virtual string[] EnumStateElements()
	{
		return States;
	}

	public abstract EZTransitionList GetTransitions(int index);

	public virtual string GetStateLabel(int index)
	{
		return null;
	}

	public virtual void SetStateLabel(int index, string label)
	{
	}

	public virtual ASCSEInfo GetStateElementInfo(int stateNum)
	{
		return new ASCSEInfo
		{
			transitions = GetTransitions(stateNum),
			stateLabel = GetStateLabel(stateNum)
		};
	}

	protected void UseStateLabel(int index)
	{
		string stateLabel = GetStateLabel(index);
		if (!(stateLabel == "[\"]") && (!(stateLabel == "") || !(spriteText == null)))
		{
			Text = stateLabel;
		}
	}

	public virtual void DoMirror()
	{
		if (!Application.isPlaying)
		{
			if (mirror == null)
			{
				mirror = new ControlBaseMirror();
				mirror.Mirror(this);
			}
			mirror.Validate(this);
			if (mirror.DidChange(this))
			{
				mirror.Mirror(this);
			}
		}
	}

	public virtual void OnDrawGizmos()
	{
		DoMirror();
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

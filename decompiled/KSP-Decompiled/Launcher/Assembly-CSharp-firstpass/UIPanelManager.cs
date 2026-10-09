using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using UnityEngine;

[Serializable]
[AddComponentMenu("EZ GUI/Management/Panel Manager")]
public class UIPanelManager : MonoBehaviour, IUIContainer, IUIObject, IEZDragDrop
{
	public enum SHOW_MODE
	{
		BringInForward,
		BringInBack,
		DismissForward,
		DismissBack
	}

	public enum MENU_DIRECTION
	{
		Forwards,
		Backwards,
		Auto
	}

	protected static UIPanelManager m_instance;

	protected List<UIPanelBase> panels = new List<UIPanelBase>();

	public UIPanelBase initialPanel;

	public bool deactivateAllButInitialAtStart;

	public bool linearNavigation;

	public bool circular;

	public bool advancePastEnd;

	protected UIPanelBase curPanel;

	protected int transitioningPanelCount;

	protected bool m_started;

	protected List<UIPanelBase> breadcrumbs = new List<UIPanelBase>();

	protected bool m_controlIsEnabled = true;

	protected IUIContainer container;

	protected EZInputDelegate inputDelegate;

	protected EZValueChangedDelegate changeDelegate;

	protected EZDragDropDelegate dragDropDelegate;

	public static UIPanelManager instance => m_instance;

	public UIPanelBase CurrentPanel => curPanel;

	public int TransitioningPanelCount => transitioningPanelCount;

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
			return false;
		}
		set
		{
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
			container = value;
		}
	}

	public object Data
	{
		get
		{
			return null;
		}
		set
		{
		}
	}

	public bool IsDraggable
	{
		get
		{
			return false;
		}
		set
		{
		}
	}

	public LayerMask DropMask
	{
		get
		{
			return -1;
		}
		set
		{
		}
	}

	public float DragOffset
	{
		get
		{
			return 0f;
		}
		set
		{
		}
	}

	public EZAnimation.EASING_TYPE CancelDragEasing
	{
		get
		{
			return EZAnimation.EASING_TYPE.Default;
		}
		set
		{
		}
	}

	public float CancelDragDuration
	{
		get
		{
			return 0f;
		}
		set
		{
		}
	}

	public bool IsDragging
	{
		get
		{
			return false;
		}
		set
		{
		}
	}

	public GameObject DropTarget
	{
		get
		{
			return null;
		}
		set
		{
		}
	}

	public bool DropHandled
	{
		get
		{
			return false;
		}
		set
		{
		}
	}

	public void OnDestroy()
	{
		m_instance = null;
	}

	public void AddChild(GameObject go)
	{
		UIPanelBase uIPanelBase = (UIPanelBase)go.GetComponent(typeof(UIPanelBase));
		if (!(uIPanelBase == null) && panels.IndexOf(uIPanelBase) < 0)
		{
			panels.Add(uIPanelBase);
			panels.Sort(UIPanelBase.CompareIndices);
			uIPanelBase.Container = this;
		}
	}

	public void RemoveChild(GameObject go)
	{
		UIPanelBase uIPanelBase = (UIPanelBase)go.GetComponent(typeof(UIPanelBase));
		if (!(uIPanelBase == null))
		{
			panels.Remove(uIPanelBase);
			uIPanelBase.Container = null;
		}
	}

	public void AddSubject(GameObject go)
	{
	}

	public void RemoveSubject(GameObject go)
	{
	}

	public void MakeChild(GameObject go)
	{
		AddChild(go);
		go.transform.parent = base.transform;
	}

	public UIPanelBase[] GetPanels()
	{
		return panels.ToArray();
	}

	public int GetPanelCount()
	{
		return panels.Count;
	}

	private void Awake()
	{
		if (m_instance == null)
		{
			m_instance = this;
		}
	}

	private IEnumerator Start()
	{
		if (m_started)
		{
			yield break;
		}
		m_started = true;
		ScanChildren();
		if (initialPanel != null)
		{
			curPanel = initialPanel;
			breadcrumbs.Add(curPanel);
		}
		if (circular)
		{
			linearNavigation = true;
		}
		if (!deactivateAllButInitialAtStart)
		{
			yield break;
		}
		yield return null;
		for (int i = 0; i < panels.Count; i++)
		{
			if (panels[i] != initialPanel && panels[i] != curPanel)
			{
				panels[i].gameObject.SetActiveRecursively(state: false);
			}
		}
	}

	protected virtual void OnEnable()
	{
		if (!m_started || !deactivateAllButInitialAtStart)
		{
			return;
		}
		for (int i = 0; i < panels.Count; i++)
		{
			if (panels[i] != curPanel)
			{
				panels[i].gameObject.SetActiveRecursively(state: false);
			}
		}
	}

	public void ScanChildren()
	{
		panels.Clear();
		Component[] componentsInChildren = base.transform.GetComponentsInChildren(typeof(UIPanelBase), includeInactive: true);
		for (int i = 0; i < componentsInChildren.Length; i++)
		{
			SetLayerRecursively(componentsInChildren[i].gameObject, base.gameObject.layer);
			UIPanelBase uIPanelBase = (UIPanelBase)componentsInChildren[i];
			if (uIPanelBase.RequestContainership(this))
			{
				panels.Add(uIPanelBase);
			}
		}
		panels.Sort(UIPanelBase.CompareIndices);
	}

	protected void DecrementTransitioningPanels(UIPanelBase p, EZTransition t)
	{
		transitioningPanelCount--;
	}

	protected void StartAndTrack(UIPanelBase p, SHOW_MODE mode)
	{
		p.StartTransition(mode);
		if (p.IsTransitioning)
		{
			p.AddTempTransitionDelegate(DecrementTransitioningPanels);
			transitioningPanelCount++;
		}
	}

	public bool MoveForward()
	{
		StartCoroutine("Start");
		int num = panels.IndexOf(curPanel);
		if (num >= panels.Count - 1)
		{
			if (!circular)
			{
				if (advancePastEnd)
				{
					if (curPanel != null)
					{
						StartAndTrack(curPanel, SHOW_MODE.DismissForward);
					}
					curPanel = null;
					if (breadcrumbs.Count > 0)
					{
						if (breadcrumbs[breadcrumbs.Count - 1] != null)
						{
							breadcrumbs.Add(null);
						}
					}
					else
					{
						breadcrumbs.Add(null);
					}
				}
				return false;
			}
			num = -1;
		}
		if (curPanel != null)
		{
			StartAndTrack(curPanel, SHOW_MODE.DismissForward);
		}
		num++;
		curPanel = panels[num];
		breadcrumbs.Add(curPanel);
		if (deactivateAllButInitialAtStart && !curPanel.gameObject.active)
		{
			curPanel.Start();
			curPanel.gameObject.SetActiveRecursively(state: true);
		}
		StartAndTrack(curPanel, SHOW_MODE.BringInForward);
		if (num >= panels.Count - 1 && !circular)
		{
			return false;
		}
		return true;
	}

	public bool MoveBack()
	{
		if (!linearNavigation)
		{
			if (breadcrumbs.Count <= 1)
			{
				if (advancePastEnd)
				{
					if (curPanel != null)
					{
						StartAndTrack(curPanel, SHOW_MODE.DismissBack);
					}
					curPanel = null;
					if (breadcrumbs.Count > 0)
					{
						if (breadcrumbs[breadcrumbs.Count - 1] != null)
						{
							breadcrumbs.Add(null);
						}
					}
					else
					{
						breadcrumbs.Add(null);
					}
				}
				return false;
			}
			if (breadcrumbs.Count != 0)
			{
				breadcrumbs.RemoveAt(breadcrumbs.Count - 1);
			}
			if (curPanel != null)
			{
				StartAndTrack(curPanel, SHOW_MODE.DismissBack);
			}
			if (breadcrumbs.Count > 0)
			{
				curPanel = breadcrumbs[breadcrumbs.Count - 1];
			}
			if (curPanel != null)
			{
				if (deactivateAllButInitialAtStart && !curPanel.gameObject.active)
				{
					curPanel.Start();
					curPanel.gameObject.SetActiveRecursively(state: true);
				}
				StartAndTrack(curPanel, SHOW_MODE.BringInBack);
			}
			if (breadcrumbs.Count <= 1)
			{
				return false;
			}
			return true;
		}
		int num = panels.IndexOf(curPanel);
		if (num <= 0)
		{
			if (!circular)
			{
				if (advancePastEnd)
				{
					if (curPanel != null)
					{
						StartAndTrack(curPanel, SHOW_MODE.DismissBack);
					}
					curPanel = null;
				}
				return false;
			}
			num = panels.Count;
		}
		if (curPanel != null)
		{
			StartAndTrack(curPanel, SHOW_MODE.DismissBack);
		}
		num--;
		curPanel = panels[num];
		if (deactivateAllButInitialAtStart && !curPanel.gameObject.active)
		{
			curPanel.Start();
			curPanel.gameObject.SetActiveRecursively(state: true);
		}
		StartAndTrack(curPanel, SHOW_MODE.BringInBack);
		if (num <= 0 && !circular)
		{
			return false;
		}
		return true;
	}

	public void BringIn(UIPanelBase panel, MENU_DIRECTION dir)
	{
		StartCoroutine("Start");
		if (!(curPanel == panel))
		{
			if (dir == MENU_DIRECTION.Auto)
			{
				dir = ((curPanel != null) ? ((curPanel.index > panel.index) ? MENU_DIRECTION.Backwards : MENU_DIRECTION.Forwards) : MENU_DIRECTION.Forwards);
			}
			SHOW_MODE mode = ((dir == MENU_DIRECTION.Forwards) ? SHOW_MODE.DismissForward : SHOW_MODE.DismissBack);
			SHOW_MODE mode2 = ((dir != 0) ? SHOW_MODE.BringInBack : SHOW_MODE.BringInForward);
			if (curPanel != null)
			{
				StartAndTrack(curPanel, mode);
			}
			curPanel = panel;
			breadcrumbs.Add(curPanel);
			if (deactivateAllButInitialAtStart && !curPanel.gameObject.active)
			{
				curPanel.Start();
				curPanel.gameObject.SetActiveRecursively(state: true);
			}
			StartAndTrack(curPanel, mode2);
		}
	}

	public void BringInImmediate(UIPanelBase panel, MENU_DIRECTION dir)
	{
		StartCoroutine("Start");
		UIPanelBase uIPanelBase = curPanel;
		if (dir == MENU_DIRECTION.Auto)
		{
			dir = ((curPanel != null) ? ((curPanel.index > panel.index) ? MENU_DIRECTION.Backwards : MENU_DIRECTION.Forwards) : MENU_DIRECTION.Forwards);
		}
		SHOW_MODE transition = ((dir == MENU_DIRECTION.Forwards) ? SHOW_MODE.DismissForward : SHOW_MODE.DismissBack);
		SHOW_MODE transition2 = ((dir != 0) ? SHOW_MODE.BringInBack : SHOW_MODE.BringInForward);
		BringIn(panel, dir);
		if (uIPanelBase != null)
		{
			uIPanelBase.GetTransition(transition).End();
		}
		if (curPanel != null)
		{
			curPanel.GetTransition(transition2).End();
		}
	}

	public void BringIn(string panelName, MENU_DIRECTION dir)
	{
		StartCoroutine("Start");
		UIPanelBase uIPanelBase = null;
		for (int i = 0; i < panels.Count; i++)
		{
			if (string.Equals(panels[i].name, panelName, StringComparison.CurrentCultureIgnoreCase))
			{
				uIPanelBase = panels[i];
				break;
			}
		}
		if (uIPanelBase != null)
		{
			BringIn(uIPanelBase, dir);
		}
	}

	public void BringIn(UIPanelBase panel)
	{
		BringIn(panel, MENU_DIRECTION.Auto);
	}

	public void BringIn(string panelName)
	{
		BringIn(panelName, MENU_DIRECTION.Auto);
	}

	public void BringIn(int panelIndex)
	{
		StartCoroutine("Start");
		for (int i = 0; i < panels.Count; i++)
		{
			if (panels[i].index == panelIndex)
			{
				BringIn(panels[i]);
				return;
			}
		}
		Debug.LogWarning("No panel found with index value of " + panelIndex);
	}

	public void BringIn(int panelIndex, MENU_DIRECTION dir)
	{
		StartCoroutine("Start");
		for (int i = 0; i < panels.Count; i++)
		{
			if (panels[i].index == panelIndex)
			{
				BringIn(panels[i], dir);
				return;
			}
		}
		Debug.LogWarning("No panel found with index value of " + panelIndex);
	}

	public void BringInImmediate(string panelName, MENU_DIRECTION dir)
	{
		StartCoroutine("Start");
		UIPanelBase uIPanelBase = null;
		for (int i = 0; i < panels.Count; i++)
		{
			if (string.Equals(panels[i].name, panelName, StringComparison.CurrentCultureIgnoreCase))
			{
				uIPanelBase = panels[i];
				break;
			}
		}
		if (uIPanelBase != null)
		{
			BringInImmediate(uIPanelBase, dir);
		}
	}

	public void BringInImmediate(UIPanelBase panel)
	{
		BringInImmediate(panel, MENU_DIRECTION.Auto);
	}

	public void BringInImmediate(string panelName)
	{
		BringInImmediate(panelName, MENU_DIRECTION.Auto);
	}

	public void BringInImmediate(int panelIndex)
	{
		StartCoroutine("Start");
		for (int i = 0; i < panels.Count; i++)
		{
			if (panels[i].index == panelIndex)
			{
				BringInImmediate(panels[i]);
				return;
			}
		}
		Debug.LogWarning("No panel found with index value of " + panelIndex);
	}

	public void BringInImmediate(int panelIndex, MENU_DIRECTION dir)
	{
		StartCoroutine("Start");
		for (int i = 0; i < panels.Count; i++)
		{
			if (panels[i].index == panelIndex)
			{
				BringInImmediate(panels[i], dir);
				return;
			}
		}
		Debug.LogWarning("No panel found with index value of " + panelIndex);
	}

	public void Dismiss(MENU_DIRECTION dir)
	{
		StartCoroutine("Start");
		if (dir == MENU_DIRECTION.Auto)
		{
			dir = MENU_DIRECTION.Backwards;
		}
		SHOW_MODE mode = ((dir == MENU_DIRECTION.Forwards) ? SHOW_MODE.DismissForward : SHOW_MODE.DismissBack);
		if (curPanel != null)
		{
			StartAndTrack(curPanel, mode);
		}
		curPanel = null;
		if (breadcrumbs.Count > 0 && breadcrumbs[breadcrumbs.Count - 1] != null)
		{
			breadcrumbs.Add(null);
		}
	}

	public void Dismiss()
	{
		Dismiss(MENU_DIRECTION.Auto);
	}

	public void DismissImmediate(MENU_DIRECTION dir)
	{
		StartCoroutine("Start");
		if (dir == MENU_DIRECTION.Auto)
		{
			dir = MENU_DIRECTION.Backwards;
		}
		SHOW_MODE transition = ((dir == MENU_DIRECTION.Forwards) ? SHOW_MODE.DismissForward : SHOW_MODE.DismissBack);
		UIPanelBase uIPanelBase = curPanel;
		Dismiss(dir);
		if (uIPanelBase != null)
		{
			uIPanelBase.GetTransition(transition).End();
		}
	}

	public void DismissImmediate()
	{
		DismissImmediate(MENU_DIRECTION.Auto);
	}

	public static void SetLayerRecursively(GameObject go, int layer)
	{
		go.layer = layer;
		foreach (Transform item in go.transform)
		{
			SetLayerRecursively(item.gameObject, layer);
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
				container = cont;
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

	public bool GotFocus()
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
	}

	public void DragUpdatePosition(POINTER_INFO ptr)
	{
	}

	public void CancelDrag()
	{
	}

	public void OnEZDragDrop_Internal(EZDragDropParams parms)
	{
		if (dragDropDelegate != null)
		{
			dragDropDelegate(parms);
		}
	}

	public void AddDragDropDelegate(EZDragDropDelegate del)
	{
		dragDropDelegate = (EZDragDropDelegate)Delegate.Combine(dragDropDelegate, del);
	}

	public void RemoveDragDropDelegate(EZDragDropDelegate del)
	{
		dragDropDelegate = (EZDragDropDelegate)Delegate.Remove(dragDropDelegate, del);
	}

	public void SetDragDropDelegate(EZDragDropDelegate del)
	{
		dragDropDelegate = del;
	}

	public void SetDragDropInternalDelegate(EZDragDropHelper.DragDrop_InternalDelegate del)
	{
	}

	public void AddDragDropInternalDelegate(EZDragDropHelper.DragDrop_InternalDelegate del)
	{
	}

	public void RemoveDragDropInternalDelegate(EZDragDropHelper.DragDrop_InternalDelegate del)
	{
	}

	public EZDragDropHelper.DragDrop_InternalDelegate GetDragDropInternalDelegate()
	{
		return null;
	}

	public static UIPanelManager Create(string name, Vector3 pos)
	{
		GameObject obj = new GameObject(name);
		obj.transform.position = pos;
		return (UIPanelManager)obj.AddComponent(typeof(UIPanelManager));
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

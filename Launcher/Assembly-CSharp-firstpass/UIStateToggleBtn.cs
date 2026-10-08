using System.Collections.Generic;
using UnityEngine;

[AddComponentMenu("EZ GUI/Controls/Toggle Button")]
public class UIStateToggleBtn : AutoSpriteControlBase
{
	protected int curStateIndex;

	protected bool stateChangeWhileDeactivated;

	public int defaultState;

	[HideInInspector]
	public TextureAnim[] states = new TextureAnim[2]
	{
		new TextureAnim("Unnamed"),
		new TextureAnim("Disabled")
	};

	[HideInInspector]
	public string[] stateLabels = new string[4] { "[\"]", "[\"]", "[\"]", "[\"]" };

	[HideInInspector]
	public EZTransitionList[] transitions = new EZTransitionList[2]
	{
		new EZTransitionList(new EZTransition[1]
		{
			new EZTransition("From Prev")
		}),
		new EZTransitionList(new EZTransition[1]
		{
			new EZTransition("From State")
		})
	};

	private EZTransition prevTransition;

	public SpriteRoot[] layers = new SpriteRoot[0];

	public MonoBehaviour scriptWithMethodToInvoke;

	public string methodToInvoke = "";

	public POINTER_INFO.INPUT_EVENT whenToInvoke = POINTER_INFO.INPUT_EVENT.TAP;

	public float delay;

	public AudioSource soundToPlay;

	public bool disableHoverEffect;

	protected int[,] stateIndices;

	protected int overLayerState;

	protected int activeLayerState;

	protected int layerState;

	public override bool controlIsEnabled
	{
		get
		{
			return m_controlIsEnabled;
		}
		set
		{
			m_controlIsEnabled = value;
			if (!value)
			{
				DisableMe();
			}
			else
			{
				SetToggleState(curStateIndex);
			}
		}
	}

	public int StateNum => curStateIndex;

	public string StateName => states[curStateIndex].name;

	public override TextureAnim[] States
	{
		get
		{
			return states;
		}
		set
		{
			states = value;
		}
	}

	public override EZTransitionList[] Transitions
	{
		get
		{
			return transitions;
		}
		set
		{
			transitions = value;
		}
	}

	public override CSpriteFrame DefaultFrame
	{
		get
		{
			if (States[defaultState].spriteFrames.Length != 0)
			{
				return States[defaultState].spriteFrames[0];
			}
			return null;
		}
	}

	public override TextureAnim DefaultState => States[defaultState];

	public override string Text
	{
		get
		{
			return base.Text;
		}
		set
		{
			bool num = spriteText == null;
			base.Text = value;
			if (!num || !(spriteText != null) || !Application.isPlaying)
			{
				return;
			}
			for (int i = 0; i < transitions.Length; i++)
			{
				for (int j = 0; j < transitions[i].list.Length; j++)
				{
					transitions[i].list[j].AddSubSubject(spriteText.gameObject);
				}
			}
		}
	}

	public override string GetStateLabel(int index)
	{
		return stateLabels[index];
	}

	public override void SetStateLabel(int index, string label)
	{
		stateLabels[index] = label;
		if (index == curStateIndex)
		{
			UseStateLabel(index);
		}
	}

	public override EZTransitionList GetTransitions(int index)
	{
		if (index >= transitions.Length)
		{
			return null;
		}
		return transitions[index];
	}

	public override void OnInput(ref POINTER_INFO ptr)
	{
		if (deleted)
		{
			return;
		}
		if (!m_controlIsEnabled || IsHidden())
		{
			base.OnInput(ref ptr);
			return;
		}
		if (inputDelegate != null)
		{
			inputDelegate(ref ptr);
		}
		if (!m_controlIsEnabled || IsHidden())
		{
			base.OnInput(ref ptr);
			return;
		}
		if (ptr.evt == whenToInvoke)
		{
			ToggleState();
			if (soundToPlay != null)
			{
				soundToPlay.PlayOneShot(soundToPlay.clip);
			}
			if (scriptWithMethodToInvoke != null)
			{
				scriptWithMethodToInvoke.Invoke(methodToInvoke, delay);
			}
		}
		switch (ptr.evt)
		{
		case POINTER_INFO.INPUT_EVENT.MOVE:
			if (!disableHoverEffect)
			{
				SetLayerState(overLayerState);
			}
			break;
		case POINTER_INFO.INPUT_EVENT.PRESS:
		case POINTER_INFO.INPUT_EVENT.DRAG:
			if (!disableHoverEffect)
			{
				SetLayerState(activeLayerState);
			}
			break;
		case POINTER_INFO.INPUT_EVENT.RELEASE:
		case POINTER_INFO.INPUT_EVENT.TAP:
			if (ptr.type != POINTER_INFO.POINTER_TYPE.TOUCHPAD && ptr.hitInfo.collider == GetComponent<Collider>() && !disableHoverEffect)
			{
				SetLayerState(overLayerState);
			}
			else
			{
				SetLayerState(curStateIndex);
			}
			break;
		case POINTER_INFO.INPUT_EVENT.MOVE_OFF:
		case POINTER_INFO.INPUT_EVENT.RELEASE_OFF:
			SetLayerState(curStateIndex);
			break;
		}
		base.OnInput(ref ptr);
	}

	protected override void Awake()
	{
		base.Awake();
		curStateIndex = defaultState;
	}

	public override void Start()
	{
		if (m_started)
		{
			return;
		}
		base.Start();
		aggregateLayers = new SpriteRoot[1][];
		aggregateLayers[0] = layers;
		if (Application.isPlaying)
		{
			stateIndices = new int[layers.Length, states.Length + 3];
			overLayerState = states.Length + 1;
			activeLayerState = states.Length + 2;
			for (int i = 0; i < states.Length; i++)
			{
				transitions[i].list[0].MainSubject = base.gameObject;
				for (int j = 0; j < layers.Length; j++)
				{
					if (layers[j] == null)
					{
						Debug.LogError("A null layer sprite was encountered on control \"" + base.name + "\". Please fill in the layer reference, or remove the empty element.");
						continue;
					}
					stateIndices[j, i] = layers[j].GetStateIndex(states[i].name);
					if (stateIndices[j, curStateIndex] != -1)
					{
						layers[j].SetState(stateIndices[j, curStateIndex]);
					}
					else
					{
						layers[j].Hide(tf: true);
					}
					if (stateIndices[j, i] != -1)
					{
						transitions[i].list[0].AddSubSubject(layers[j].gameObject);
					}
				}
				if (spriteText != null)
				{
					transitions[i].list[0].AddSubSubject(spriteText.gameObject);
				}
			}
			for (int j = 0; j < layers.Length; j++)
			{
				stateIndices[j, overLayerState] = layers[j].GetStateIndex("Over");
				stateIndices[j, activeLayerState] = layers[j].GetStateIndex("Active");
			}
			if (GetComponent<Collider>() == null)
			{
				AddCollider();
			}
			SetToggleState(curStateIndex, suppressTransition: true);
		}
		if (managed && m_hidden)
		{
			Hide(tf: true);
		}
	}

	public override void Copy(SpriteRoot s)
	{
		Copy(s, ControlCopyFlags.All);
	}

	public override void Copy(SpriteRoot s, ControlCopyFlags flags)
	{
		base.Copy(s, flags);
		if (!(s is UIStateToggleBtn))
		{
			return;
		}
		UIStateToggleBtn uIStateToggleBtn = (UIStateToggleBtn)s;
		if ((flags & ControlCopyFlags.Settings) == ControlCopyFlags.Settings)
		{
			defaultState = uIStateToggleBtn.defaultState;
		}
		if ((flags & ControlCopyFlags.State) == ControlCopyFlags.State)
		{
			prevTransition = uIStateToggleBtn.prevTransition;
			if (Application.isPlaying)
			{
				SetToggleState(uIStateToggleBtn.StateNum);
			}
		}
		if ((flags & ControlCopyFlags.Invocation) == ControlCopyFlags.Invocation)
		{
			scriptWithMethodToInvoke = uIStateToggleBtn.scriptWithMethodToInvoke;
			methodToInvoke = uIStateToggleBtn.methodToInvoke;
			whenToInvoke = uIStateToggleBtn.whenToInvoke;
			delay = uIStateToggleBtn.delay;
		}
		if ((flags & ControlCopyFlags.Sound) == ControlCopyFlags.Sound)
		{
			soundToPlay = uIStateToggleBtn.soundToPlay;
		}
	}

	public int ToggleState()
	{
		SetToggleState(curStateIndex + 1);
		return curStateIndex;
	}

	public virtual void SetToggleState(int s, bool suppressTransition)
	{
		curStateIndex = s % (states.Length - 1);
		if (!base.gameObject.active)
		{
			stateChangeWhileDeactivated = true;
			if (changeDelegate != null)
			{
				changeDelegate(this);
			}
			return;
		}
		SetState(curStateIndex);
		UseStateLabel(curStateIndex);
		UpdateCollider();
		SetLayerState(curStateIndex);
		for (int i = 0; i < layers.Length; i++)
		{
			if (-1 != stateIndices[i, curStateIndex])
			{
				layers[i].Hide(IsHidden());
				layers[i].SetState(stateIndices[i, curStateIndex]);
			}
			else
			{
				layers[i].Hide(tf: true);
			}
		}
		if (prevTransition != null)
		{
			prevTransition.StopSafe();
		}
		if (!suppressTransition)
		{
			transitions[curStateIndex].list[0].Start();
			prevTransition = transitions[curStateIndex].list[0];
		}
		if (changeDelegate != null && !stateChangeWhileDeactivated)
		{
			changeDelegate(this);
		}
	}

	public virtual void SetToggleState(int s)
	{
		SetToggleState(s, suppressTransition: false);
	}

	public virtual void SetToggleState(string stateName, bool suppressTransition)
	{
		for (int i = 0; i < states.Length; i++)
		{
			if (states[i].name == stateName)
			{
				SetToggleState(i, suppressTransition);
				break;
			}
		}
	}

	public virtual void SetToggleState(string stateName)
	{
		SetToggleState(stateName, suppressTransition: false);
	}

	protected void SetLayerState(int s)
	{
		if (s == layerState)
		{
			return;
		}
		layerState = s;
		for (int i = 0; i < layers.Length; i++)
		{
			if (-1 != stateIndices[i, layerState])
			{
				layers[i].Hide(tf: false);
				layers[i].SetState(stateIndices[i, layerState]);
			}
			else
			{
				layers[i].Hide(tf: true);
			}
		}
	}

	protected void DisableMe()
	{
		SetState(states.Length - 1);
		UseStateLabel(states.Length - 1);
		for (int i = 0; i < layers.Length; i++)
		{
			if (stateIndices[i, states.Length - 1] != -1)
			{
				layers[i].SetState(stateIndices[i, states.Length - 1]);
			}
		}
		if (prevTransition != null)
		{
			prevTransition.StopSafe();
		}
		transitions[states.Length - 1].list[0].Start();
		prevTransition = transitions[states.Length - 1].list[0];
	}

	protected override void OnEnable()
	{
		base.OnEnable();
		if (stateChangeWhileDeactivated)
		{
			SetToggleState(curStateIndex);
			stateChangeWhileDeactivated = false;
		}
	}

	public override void InitUVs()
	{
		if (states != null && defaultState <= states.Length - 1 && states[defaultState].spriteFrames.Length != 0)
		{
			frameInfo.Copy(states[defaultState].spriteFrames[0]);
		}
		base.InitUVs();
	}

	public override int DrawPreStateSelectGUI(int selState, bool inspector)
	{
		GUILayout.BeginHorizontal(GUILayout.MaxWidth(50f));
		if (GUILayout.Button(inspector ? "+" : "Add State", inspector ? "ToolbarButton" : "Button"))
		{
			List<TextureAnim> list = new List<TextureAnim>();
			list.AddRange(states);
			list.Insert(states.Length - 1, new TextureAnim("State " + (states.Length - 1)));
			states = list.ToArray();
			List<EZTransitionList> list2 = new List<EZTransitionList>();
			list2.AddRange(transitions);
			list2.Insert(transitions.Length - 1, new EZTransitionList(new EZTransition[1]
			{
				new EZTransition("From Prev")
			}));
			transitions = list2.ToArray();
			List<string> list3 = new List<string>();
			list3.AddRange(stateLabels);
			list3.Insert(stateLabels.Length - 1, "[\"]");
			stateLabels = list3.ToArray();
		}
		if (states.Length > 2 && selState != states.Length - 1)
		{
			if (GUILayout.Button(inspector ? "-" : "Delete State", inspector ? "ToolbarButton" : "Button"))
			{
				List<TextureAnim> list4 = new List<TextureAnim>();
				list4.AddRange(states);
				list4.RemoveAt(selState);
				states = list4.ToArray();
				List<EZTransitionList> list5 = new List<EZTransitionList>();
				list5.AddRange(transitions);
				list5.RemoveAt(selState);
				transitions = list5.ToArray();
				List<string> list6 = new List<string>();
				list6.AddRange(stateLabels);
				list6.RemoveAt(selState);
				stateLabels = list6.ToArray();
			}
			defaultState %= states.Length;
		}
		if (inspector)
		{
			GUILayout.FlexibleSpace();
		}
		GUILayout.EndHorizontal();
		return 14;
	}

	public override int DrawPostStateSelectGUI(int selState)
	{
		GUILayout.BeginHorizontal(GUILayout.MaxWidth(50f));
		GUILayout.Space(20f);
		GUILayout.Label("State Name:");
		if (selState < states.Length - 1)
		{
			states[selState].name = GUILayout.TextField(states[selState].name);
		}
		else
		{
			GUILayout.TextField(states[selState].name);
		}
		GUILayout.EndHorizontal();
		return 28;
	}

	public override void DrawPreTransitionUI(int selState, IGUIScriptSelector gui)
	{
		scriptWithMethodToInvoke = gui.DrawScriptSelection(scriptWithMethodToInvoke, ref methodToInvoke);
	}

	public static UIStateToggleBtn Create(string name, Vector3 pos)
	{
		GameObject obj = new GameObject(name);
		obj.transform.position = pos;
		return (UIStateToggleBtn)obj.AddComponent(typeof(UIStateToggleBtn));
	}

	public static UIStateToggleBtn Create(string name, Vector3 pos, Quaternion rotation)
	{
		GameObject obj = new GameObject(name);
		obj.transform.position = pos;
		obj.transform.rotation = rotation;
		return (UIStateToggleBtn)obj.AddComponent(typeof(UIStateToggleBtn));
	}
}

using System;
using UnityEngine;

[Serializable]
[AddComponentMenu("EZ GUI/Panels/Panel")]
public class UIPanel : UIPanelBase
{
	[HideInInspector]
	public EZTransitionList transitions = new EZTransitionList(new EZTransition[4]
	{
		new EZTransition("Bring In Forward"),
		new EZTransition("Bring In Back"),
		new EZTransition("Dismiss Forward"),
		new EZTransition("Dismiss Back")
	});

	public override EZTransitionList Transitions => transitions;

	public static UIPanel Create(string name, Vector3 pos)
	{
		GameObject obj = new GameObject(name);
		obj.transform.position = pos;
		return (UIPanel)obj.AddComponent(typeof(UIPanel));
	}

	public static UIPanel Create(string name, Vector3 pos, Quaternion rotation)
	{
		GameObject obj = new GameObject(name);
		obj.transform.position = pos;
		obj.transform.rotation = rotation;
		return (UIPanel)obj.AddComponent(typeof(UIPanel));
	}
}

using UnityEngine;

[AddComponentMenu("EZ GUI/Controls/Action Button")]
public class UIActionBtn : UIButton
{
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
		if (repeat)
		{
			UIManager.instance.RayActive = ((base.controlState == CONTROL_STATE.ACTIVE) ? UIManager.RAY_ACTIVE_STATE.Constant : UIManager.RAY_ACTIVE_STATE.Inactive);
		}
		else if (ptr.evt == whenToInvoke)
		{
			UIManager.instance.RayActive = UIManager.RAY_ACTIVE_STATE.Momentary;
		}
		base.OnInput(ref ptr);
	}

	public new static UIActionBtn Create(string name, Vector3 pos)
	{
		GameObject obj = new GameObject(name);
		obj.transform.position = pos;
		return (UIActionBtn)obj.AddComponent(typeof(UIActionBtn));
	}

	public new static UIActionBtn Create(string name, Vector3 pos, Quaternion rotation)
	{
		GameObject obj = new GameObject(name);
		obj.transform.position = pos;
		obj.transform.rotation = rotation;
		return (UIActionBtn)obj.AddComponent(typeof(UIActionBtn));
	}
}

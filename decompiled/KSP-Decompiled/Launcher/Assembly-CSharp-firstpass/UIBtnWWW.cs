using UnityEngine;

[AddComponentMenu("EZ GUI/Controls/Weblink Button")]
public class UIBtnWWW : UIButton
{
	public string URL;

	public override void OnInput(ref POINTER_INFO ptr)
	{
		if (!deleted)
		{
			base.OnInput(ref ptr);
			if (m_controlIsEnabled && !IsHidden() && ptr.evt == whenToInvoke)
			{
				Invoke("DoURL", delay);
			}
		}
	}

	protected void DoURL()
	{
		Application.OpenURL(URL);
	}

	public override void Copy(SpriteRoot s)
	{
		Copy(s, ControlCopyFlags.All);
	}

	public override void Copy(SpriteRoot s, ControlCopyFlags flags)
	{
		base.Copy(s, flags);
		if (s is UIBtnWWW)
		{
			UIBtnWWW uIBtnWWW = (UIBtnWWW)s;
			if ((flags & ControlCopyFlags.Settings) == ControlCopyFlags.Settings)
			{
				URL = uIBtnWWW.URL;
			}
		}
	}

	public new static UIBtnWWW Create(string name, Vector3 pos)
	{
		GameObject obj = new GameObject(name);
		obj.transform.position = pos;
		return (UIBtnWWW)obj.AddComponent(typeof(UIBtnWWW));
	}

	public new static UIBtnWWW Create(string name, Vector3 pos, Quaternion rotation)
	{
		GameObject obj = new GameObject(name);
		obj.transform.position = pos;
		obj.transform.rotation = rotation;
		return (UIBtnWWW)obj.AddComponent(typeof(UIBtnWWW));
	}
}

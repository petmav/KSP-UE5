using KSP.UI;
using UnityEngine;

[RequireComponent(typeof(UISelectableGridLayoutGroupItem))]
public class UIMarquee_ActionField : UIMarquee
{
	protected UISelectableGridLayoutGroupItem fieldItem;

	protected override void Start()
	{
		fieldItem = GetComponent<UISelectableGridLayoutGroupItem>();
		GameEvents.onGUIActionGroupClosed.Add(OnPartActionUIDismiss);
		base.Start();
	}

	protected override void OnDestroy()
	{
		GameEvents.onGUIActionGroupClosed.Remove(OnPartActionUIDismiss);
		base.OnDestroy();
	}

	protected void OnPartActionUIDismiss()
	{
		StopMarquee();
	}
}

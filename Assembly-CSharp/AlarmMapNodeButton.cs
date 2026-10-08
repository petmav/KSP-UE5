using KSP.UI.Screens;
using UnityEngine;
using UnityEngine.UI;

public class AlarmMapNodeButton : MonoBehaviour
{
	internal MapObject mapObject;

	[SerializeField]
	private Button addAlarmButton;

	private bool hover;

	public bool Hover => hover;

	public void OnPointerEnter()
	{
		mapObject.uiNode.addAlarmHover = true;
		hover = true;
	}

	public void OnPointerExit()
	{
		mapObject.uiNode.addAlarmHover = false;
		mapObject.uiNode.OnAddAlarmPointerExit();
		hover = false;
	}

	private void Awake()
	{
		addAlarmButton.onClick.AddListener(OnAddAlarm);
	}

	private void OnDestroy()
	{
		addAlarmButton.onClick.RemoveListener(OnAddAlarm);
	}

	private void OnAddAlarm()
	{
		if (!AlarmClockScenario.MapNodeDefined(mapObject.type))
		{
			return;
		}
		AlarmTypeBase alarmTypeBase = AlarmClockScenario.CreateAlarmByMapNodeType(mapObject.type);
		if (alarmTypeBase != null)
		{
			if (mapObject.vesselRef != null)
			{
				alarmTypeBase.vesselId = mapObject.vesselRef.persistentId;
			}
			if (alarmTypeBase.InitializeFromMapObject(mapObject))
			{
				alarmTypeBase.OnScenarioUpdate();
				if (AlarmClockScenario.AddAlarm(alarmTypeBase))
				{
					AlarmClockApp.EditAlarm(alarmTypeBase);
				}
			}
		}
		OnPointerExit();
	}
}

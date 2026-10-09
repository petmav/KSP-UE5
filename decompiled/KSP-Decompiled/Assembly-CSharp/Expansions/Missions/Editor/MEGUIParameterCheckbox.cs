using UnityEngine.UI;

namespace Expansions.Missions.Editor
{
	[MEGUI_Checkbox]
	public class MEGUIParameterCheckbox : MEGUIParameter
	{
		public Toggle toggle;

		public override bool IsInteractable
		{
			get
			{
				return toggle.interactable;
			}
			set
			{
				toggle.interactable = value;
			}
		}

		public bool FieldValue
		{
			get
			{
				return field.GetValue<bool>();
			}
			set
			{
				MissionEditorHistory.PushUndoAction(this, OnHistoryValueChange);
				if (((MEGUI_Checkbox)field.Attribute).autoDisable)
				{
					toggle.interactable = !value;
					toggle.isOn = value;
				}
				field.SetValue(value);
			}
		}

		protected override void Setup(string name)
		{
			title.text = name;
			toggle.isOn = FieldValue;
			if (((MEGUI_Checkbox)field.Attribute).autoDisable)
			{
				toggle.interactable = !toggle.isOn;
			}
			toggle.onValueChanged.AddListener(OnParameterValueChanged);
		}

		protected override void ResetDefaultValue(string value)
		{
			bool result = false;
			if (bool.TryParse(value, out result))
			{
				FieldValue = result;
			}
		}

		public override void RefreshUI()
		{
			toggle.isOn = FieldValue;
		}

		private void OnParameterValueChanged(bool value)
		{
			FieldValue = value;
			UpdateNodeBodyUI();
		}

		public void OnHistoryValueChange(ConfigNode data, HistoryType type)
		{
			bool value = false;
			if (data.TryGetValue("value", ref value))
			{
				toggle.isOn = value;
			}
		}
	}
}

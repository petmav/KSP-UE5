using System;

namespace UnityEngine.Analytics;

[Serializable]
public class ValueProperty
{
	public enum PropertyType
	{
		Disabled,
		Static,
		Dynamic
	}

	[SerializeField]
	private bool m_EditingCustomValue;

	[SerializeField]
	private int m_PopupIndex;

	[SerializeField]
	private string m_CustomValue;

	[SerializeField]
	private bool m_FixedType;

	[SerializeField]
	private string m_EnumType;

	[SerializeField]
	private bool m_EnumTypeIsCustomizable = true;

	[SerializeField]
	private bool m_CanDisable;

	[SerializeField]
	private PropertyType m_PropertyType = PropertyType.Static;

	[SerializeField]
	private string m_ValueType;

	[SerializeField]
	private string m_Value = string.Empty;

	[SerializeField]
	private TrackableField m_Target;

	public string valueType
	{
		get
		{
			return m_ValueType;
		}
		set
		{
			m_ValueType = value;
		}
	}

	public string propertyValue
	{
		get
		{
			if (m_PropertyType == PropertyType.Dynamic && m_Target != null)
			{
				return m_Target.GetValue()?.ToString().Trim();
			}
			return (m_Value != null) ? m_Value.Trim() : null;
		}
	}

	public TrackableField target => m_Target;

	public bool IsValid()
	{
		return m_PropertyType switch
		{
			PropertyType.Static => !string.IsNullOrEmpty(m_Value) || (object)Type.GetType(m_ValueType) != typeof(string), 
			PropertyType.Dynamic => m_Target != null && m_Target.GetValue() != null, 
			_ => false, 
		};
	}
}

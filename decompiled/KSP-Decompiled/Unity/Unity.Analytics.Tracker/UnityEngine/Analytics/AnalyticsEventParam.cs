using System;

namespace UnityEngine.Analytics;

[Serializable]
public class AnalyticsEventParam
{
	public enum RequirementType
	{
		None,
		Required,
		Optional
	}

	[SerializeField]
	private RequirementType m_RequirementType;

	[SerializeField]
	private string m_GroupID;

	[SerializeField]
	private string m_Tooltip = string.Empty;

	[SerializeField]
	private string m_Name;

	[SerializeField]
	private ValueProperty m_Value;

	public RequirementType requirementType => m_RequirementType;

	public string groupID => m_GroupID;

	public ValueProperty valueProperty => m_Value;

	public string name => m_Name;

	public object value => m_Value.propertyValue;

	public AnalyticsEventParam(string name)
	{
		m_Name = name.Trim();
	}
}

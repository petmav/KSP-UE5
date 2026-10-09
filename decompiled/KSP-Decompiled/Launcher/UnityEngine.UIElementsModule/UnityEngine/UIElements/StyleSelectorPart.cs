using System;

namespace UnityEngine.UIElements;

[Serializable]
internal struct StyleSelectorPart
{
	[SerializeField]
	private string m_Value;

	[SerializeField]
	private StyleSelectorType m_Type;

	internal object tempData;

	public string value
	{
		get
		{
			return m_Value;
		}
		internal set
		{
			m_Value = value;
		}
	}

	public StyleSelectorType type
	{
		get
		{
			return m_Type;
		}
		internal set
		{
			m_Type = value;
		}
	}

	public override string ToString()
	{
		return UnityString.Format("[StyleSelectorPart: value={0}, type={1}]", value, type);
	}

	public static StyleSelectorPart CreateClass(string className)
	{
		StyleSelectorPart result = default(StyleSelectorPart);
		result.m_Type = StyleSelectorType.Class;
		result.m_Value = className;
		return result;
	}

	public static StyleSelectorPart CreatePseudoClass(string className)
	{
		StyleSelectorPart result = default(StyleSelectorPart);
		result.m_Type = StyleSelectorType.PseudoClass;
		result.m_Value = className;
		return result;
	}

	public static StyleSelectorPart CreateId(string Id)
	{
		StyleSelectorPart result = default(StyleSelectorPart);
		result.m_Type = StyleSelectorType.ID;
		result.m_Value = Id;
		return result;
	}

	public static StyleSelectorPart CreateType(Type t)
	{
		StyleSelectorPart result = default(StyleSelectorPart);
		result.m_Type = StyleSelectorType.Type;
		result.m_Value = t.Name;
		return result;
	}

	public static StyleSelectorPart CreateType(string typeName)
	{
		StyleSelectorPart result = default(StyleSelectorPart);
		result.m_Type = StyleSelectorType.Type;
		result.m_Value = typeName;
		return result;
	}

	public static StyleSelectorPart CreatePredicate(object predicate)
	{
		StyleSelectorPart result = default(StyleSelectorPart);
		result.m_Type = StyleSelectorType.Predicate;
		result.tempData = predicate;
		return result;
	}

	public static StyleSelectorPart CreateWildCard()
	{
		StyleSelectorPart result = default(StyleSelectorPart);
		result.m_Type = StyleSelectorType.Wildcard;
		return result;
	}
}

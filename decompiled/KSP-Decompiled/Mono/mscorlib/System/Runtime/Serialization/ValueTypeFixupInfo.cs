using System.Reflection;

namespace System.Runtime.Serialization;

internal class ValueTypeFixupInfo
{
	private long m_containerID;

	private FieldInfo m_parentField;

	private int[] m_parentIndex;

	public long ContainerID => m_containerID;

	public FieldInfo ParentField => m_parentField;

	public int[] ParentIndex => m_parentIndex;

	public ValueTypeFixupInfo(long containerID, FieldInfo member, int[] parentIndex)
	{
		if (member == null && parentIndex == null)
		{
			throw new ArgumentException(Environment.GetResourceString("When supplying the ID of a containing object, the FieldInfo that identifies the current field within that object must also be supplied."));
		}
		if (containerID == 0L && member == null)
		{
			m_containerID = containerID;
			m_parentField = member;
			m_parentIndex = parentIndex;
		}
		if (member != null)
		{
			if (parentIndex != null)
			{
				throw new ArgumentException(Environment.GetResourceString("Cannot supply both a MemberInfo and an Array to indicate the parent of a value type."));
			}
			if (member.FieldType.IsValueType && containerID == 0L)
			{
				throw new ArgumentException(Environment.GetResourceString("When supplying a FieldInfo for fixing up a nested type, a valid ID for that containing object must also be supplied."));
			}
		}
		m_containerID = containerID;
		m_parentField = member;
		m_parentIndex = parentIndex;
	}
}

using System.Text;
using Unity;

namespace System.Xml.Serialization;

public class XmlMembersMapping : XmlMapping
{
	private XmlMemberMapping[] mappings;

	public string TypeName => base.Accessor.Mapping.TypeName;

	public string TypeNamespace => base.Accessor.Mapping.Namespace;

	public XmlMemberMapping this[int index] => mappings[index];

	public int Count => mappings.Length;

	internal XmlMembersMapping(TypeScope scope, ElementAccessor accessor, XmlMappingAccess access)
		: base(scope, accessor, access)
	{
		MembersMapping membersMapping = (MembersMapping)accessor.Mapping;
		StringBuilder stringBuilder = new StringBuilder();
		stringBuilder.Append(":");
		mappings = new XmlMemberMapping[membersMapping.Members.Length];
		for (int i = 0; i < mappings.Length; i++)
		{
			if (membersMapping.Members[i].TypeDesc.Type != null)
			{
				stringBuilder.Append(XmlMapping.GenerateKey(membersMapping.Members[i].TypeDesc.Type, null, null));
				stringBuilder.Append(":");
			}
			mappings[i] = new XmlMemberMapping(membersMapping.Members[i]);
		}
		SetKeyInternal(stringBuilder.ToString());
	}

	internal XmlMembersMapping()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

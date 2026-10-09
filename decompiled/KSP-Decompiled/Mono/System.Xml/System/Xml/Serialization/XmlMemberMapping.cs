using System.CodeDom.Compiler;
using Unity;

namespace System.Xml.Serialization;

public class XmlMemberMapping
{
	private MemberMapping mapping;

	internal MemberMapping Mapping => mapping;

	internal Accessor Accessor => mapping.Accessor;

	public bool Any => Accessor.Any;

	public string ElementName => Accessor.UnescapeName(Accessor.Name);

	public string XsdElementName => Accessor.Name;

	public string Namespace => Accessor.Namespace;

	public string MemberName => mapping.Name;

	public string TypeName
	{
		get
		{
			if (Accessor.Mapping == null)
			{
				return string.Empty;
			}
			return Accessor.Mapping.TypeName;
		}
	}

	public string TypeNamespace
	{
		get
		{
			if (Accessor.Mapping == null)
			{
				return null;
			}
			return Accessor.Mapping.Namespace;
		}
	}

	public string TypeFullName => mapping.TypeDesc.FullName;

	public bool CheckSpecified => mapping.CheckSpecified != SpecifiedAccessor.None;

	internal bool IsNullable => mapping.IsNeedNullable;

	internal XmlMemberMapping(MemberMapping mapping)
	{
		this.mapping = mapping;
	}

	public string GenerateTypeName(CodeDomProvider codeProvider)
	{
		return mapping.GetTypeName(codeProvider);
	}

	internal XmlMemberMapping()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

namespace System.Xml.Serialization;

internal class StructMapping : TypeMapping, INameScope
{
	private MemberMapping[] members;

	private StructMapping baseMapping;

	private StructMapping derivedMappings;

	private StructMapping nextDerivedMapping;

	private MemberMapping xmlnsMember;

	private bool hasSimpleContent;

	private bool openModel;

	private bool isSequence;

	private NameTable elements;

	private NameTable attributes;

	private CodeIdentifiers scope;

	internal StructMapping BaseMapping
	{
		get
		{
			return baseMapping;
		}
		set
		{
			baseMapping = value;
			if (!base.IsAnonymousType && baseMapping != null)
			{
				nextDerivedMapping = baseMapping.derivedMappings;
				baseMapping.derivedMappings = this;
			}
			if (!value.isSequence || isSequence)
			{
				return;
			}
			isSequence = true;
			if (baseMapping.IsSequence)
			{
				for (StructMapping structMapping = derivedMappings; structMapping != null; structMapping = structMapping.NextDerivedMapping)
				{
					structMapping.SetSequence();
				}
			}
		}
	}

	internal StructMapping DerivedMappings => derivedMappings;

	internal bool IsFullyInitialized
	{
		get
		{
			if (baseMapping != null)
			{
				return Members != null;
			}
			return false;
		}
	}

	internal NameTable LocalElements
	{
		get
		{
			if (elements == null)
			{
				elements = new NameTable();
			}
			return elements;
		}
	}

	internal NameTable LocalAttributes
	{
		get
		{
			if (attributes == null)
			{
				attributes = new NameTable();
			}
			return attributes;
		}
	}

	object INameScope.this[string name, string ns]
	{
		get
		{
			object obj = LocalElements[name, ns];
			if (obj != null)
			{
				return obj;
			}
			if (baseMapping != null)
			{
				return ((INameScope)baseMapping)[name, ns];
			}
			return null;
		}
		set
		{
			LocalElements[name, ns] = value;
		}
	}

	internal StructMapping NextDerivedMapping => nextDerivedMapping;

	internal bool HasSimpleContent => hasSimpleContent;

	internal bool HasXmlnsMember
	{
		get
		{
			for (StructMapping structMapping = this; structMapping != null; structMapping = structMapping.BaseMapping)
			{
				if (structMapping.XmlnsMember != null)
				{
					return true;
				}
			}
			return false;
		}
	}

	internal MemberMapping[] Members
	{
		get
		{
			return members;
		}
		set
		{
			members = value;
		}
	}

	internal MemberMapping XmlnsMember
	{
		get
		{
			return xmlnsMember;
		}
		set
		{
			xmlnsMember = value;
		}
	}

	internal bool IsOpenModel
	{
		get
		{
			return openModel;
		}
		set
		{
			openModel = value;
		}
	}

	internal CodeIdentifiers Scope
	{
		get
		{
			if (scope == null)
			{
				scope = new CodeIdentifiers();
			}
			return scope;
		}
		set
		{
			scope = value;
		}
	}

	internal bool HasElements
	{
		get
		{
			if (elements != null)
			{
				return elements.Values.Count > 0;
			}
			return false;
		}
	}

	internal bool IsSequence
	{
		get
		{
			if (isSequence)
			{
				return !base.TypeDesc.IsRoot;
			}
			return false;
		}
		set
		{
			isSequence = value;
		}
	}

	internal MemberMapping FindDeclaringMapping(MemberMapping member, out StructMapping declaringMapping, string parent)
	{
		declaringMapping = null;
		if (BaseMapping != null)
		{
			MemberMapping memberMapping = BaseMapping.FindDeclaringMapping(member, out declaringMapping, parent);
			if (memberMapping != null)
			{
				return memberMapping;
			}
		}
		if (members == null)
		{
			return null;
		}
		for (int i = 0; i < members.Length; i++)
		{
			if (members[i].Name == member.Name)
			{
				if (members[i].TypeDesc != member.TypeDesc)
				{
					throw new InvalidOperationException(Res.GetString("Member {0}.{1} of type {2} hides base class member {3}.{4} of type {5}. Use XmlElementAttribute or XmlAttributeAttribute to specify a new name.", parent, member.Name, member.TypeDesc.FullName, base.TypeName, members[i].Name, members[i].TypeDesc.FullName));
				}
				if (!members[i].Match(member))
				{
					throw new InvalidOperationException(Res.GetString("Member '{0}.{1}' hides inherited member '{2}.{3}', but has different custom attributes.", parent, member.Name, base.TypeName, members[i].Name));
				}
				declaringMapping = this;
				return members[i];
			}
		}
		return null;
	}

	internal bool Declares(MemberMapping member, string parent)
	{
		StructMapping declaringMapping;
		return FindDeclaringMapping(member, out declaringMapping, parent) != null;
	}

	internal void SetContentModel(TextAccessor text, bool hasElements)
	{
		if (BaseMapping == null || BaseMapping.TypeDesc.IsRoot)
		{
			hasSimpleContent = !hasElements && text != null && !text.Mapping.IsList;
		}
		else if (BaseMapping.HasSimpleContent)
		{
			if (text != null || hasElements)
			{
				throw new InvalidOperationException(Res.GetString("Cannot serialize object of type '{0}'. Base type '{1}' has simpleContent and can only be extended by adding XmlAttribute elements. Please consider changing XmlText member of the base class to string array.", base.TypeDesc.FullName, BaseMapping.TypeDesc.FullName));
			}
			hasSimpleContent = true;
		}
		else
		{
			hasSimpleContent = false;
		}
		if (!hasSimpleContent && text != null && !text.Mapping.TypeDesc.CanBeTextValue)
		{
			throw new InvalidOperationException(Res.GetString("Cannot serialize object of type '{0}'. Consider changing type of XmlText member '{0}.{1}' from {2} to string or string array.", base.TypeDesc.FullName, text.Name, text.Mapping.TypeDesc.FullName));
		}
	}

	internal bool HasExplicitSequence()
	{
		if (members != null)
		{
			for (int i = 0; i < members.Length; i++)
			{
				if (members[i].IsParticle && members[i].IsSequence)
				{
					return true;
				}
			}
		}
		if (baseMapping != null)
		{
			return baseMapping.HasExplicitSequence();
		}
		return false;
	}

	internal void SetSequence()
	{
		if (!base.TypeDesc.IsRoot)
		{
			StructMapping structMapping = this;
			while (!structMapping.BaseMapping.IsSequence && structMapping.BaseMapping != null && !structMapping.BaseMapping.TypeDesc.IsRoot)
			{
				structMapping = structMapping.BaseMapping;
			}
			structMapping.IsSequence = true;
			for (StructMapping structMapping2 = structMapping.DerivedMappings; structMapping2 != null; structMapping2 = structMapping2.NextDerivedMapping)
			{
				structMapping2.SetSequence();
			}
		}
	}
}

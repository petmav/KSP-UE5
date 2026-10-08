namespace System.Xml.Linq;

public class XDocumentType : XNode
{
	private string name;

	private string publicId;

	private string systemId;

	private string internalSubset;

	private IDtdInfo dtdInfo;

	public string InternalSubset
	{
		get
		{
			return internalSubset;
		}
		set
		{
			bool num = NotifyChanging(this, XObjectChangeEventArgs.Value);
			internalSubset = value;
			if (num)
			{
				NotifyChanged(this, XObjectChangeEventArgs.Value);
			}
		}
	}

	public string Name
	{
		get
		{
			return name;
		}
		set
		{
			value = XmlConvert.VerifyName(value);
			bool num = NotifyChanging(this, XObjectChangeEventArgs.Name);
			name = value;
			if (num)
			{
				NotifyChanged(this, XObjectChangeEventArgs.Name);
			}
		}
	}

	public override XmlNodeType NodeType => XmlNodeType.DocumentType;

	public string PublicId
	{
		get
		{
			return publicId;
		}
		set
		{
			bool num = NotifyChanging(this, XObjectChangeEventArgs.Value);
			publicId = value;
			if (num)
			{
				NotifyChanged(this, XObjectChangeEventArgs.Value);
			}
		}
	}

	public string SystemId
	{
		get
		{
			return systemId;
		}
		set
		{
			bool num = NotifyChanging(this, XObjectChangeEventArgs.Value);
			systemId = value;
			if (num)
			{
				NotifyChanged(this, XObjectChangeEventArgs.Value);
			}
		}
	}

	internal IDtdInfo DtdInfo => dtdInfo;

	public XDocumentType(string name, string publicId, string systemId, string internalSubset)
	{
		this.name = XmlConvert.VerifyName(name);
		this.publicId = publicId;
		this.systemId = systemId;
		this.internalSubset = internalSubset;
	}

	public XDocumentType(XDocumentType other)
	{
		if (other == null)
		{
			throw new ArgumentNullException("other");
		}
		name = other.name;
		publicId = other.publicId;
		systemId = other.systemId;
		internalSubset = other.internalSubset;
		dtdInfo = other.dtdInfo;
	}

	internal XDocumentType(XmlReader r)
	{
		name = r.Name;
		publicId = r.GetAttribute("PUBLIC");
		systemId = r.GetAttribute("SYSTEM");
		internalSubset = r.Value;
		dtdInfo = r.DtdInfo;
		r.Read();
	}

	internal XDocumentType(string name, string publicId, string systemId, string internalSubset, IDtdInfo dtdInfo)
		: this(name, publicId, systemId, internalSubset)
	{
		this.dtdInfo = dtdInfo;
	}

	public override void WriteTo(XmlWriter writer)
	{
		if (writer == null)
		{
			throw new ArgumentNullException("writer");
		}
		writer.WriteDocType(name, publicId, systemId, internalSubset);
	}

	internal override XNode CloneNode()
	{
		return new XDocumentType(this);
	}

	internal override bool DeepEquals(XNode node)
	{
		if (node is XDocumentType xDocumentType && name == xDocumentType.name && publicId == xDocumentType.publicId && systemId == xDocumentType.SystemId)
		{
			return internalSubset == xDocumentType.internalSubset;
		}
		return false;
	}

	internal override int GetDeepHashCode()
	{
		return name.GetHashCode() ^ ((publicId != null) ? publicId.GetHashCode() : 0) ^ ((systemId != null) ? systemId.GetHashCode() : 0) ^ ((internalSubset != null) ? internalSubset.GetHashCode() : 0);
	}
}

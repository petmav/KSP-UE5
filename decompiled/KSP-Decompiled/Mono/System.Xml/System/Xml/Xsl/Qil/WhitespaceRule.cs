using System.Xml.Xsl.Runtime;

namespace System.Xml.Xsl.Qil;

internal class WhitespaceRule
{
	private string localName;

	private string namespaceName;

	private bool preserveSpace;

	public string LocalName
	{
		get
		{
			return localName;
		}
		set
		{
			localName = value;
		}
	}

	public string NamespaceName
	{
		get
		{
			return namespaceName;
		}
		set
		{
			namespaceName = value;
		}
	}

	public bool PreserveSpace => preserveSpace;

	protected WhitespaceRule()
	{
	}

	public WhitespaceRule(string localName, string namespaceName, bool preserveSpace)
	{
		Init(localName, namespaceName, preserveSpace);
	}

	protected void Init(string localName, string namespaceName, bool preserveSpace)
	{
		this.localName = localName;
		this.namespaceName = namespaceName;
		this.preserveSpace = preserveSpace;
	}

	public void GetObjectData(XmlQueryDataWriter writer)
	{
		writer.WriteStringQ(localName);
		writer.WriteStringQ(namespaceName);
		writer.Write(preserveSpace);
	}

	public WhitespaceRule(XmlQueryDataReader reader)
	{
		localName = reader.ReadStringQ();
		namespaceName = reader.ReadStringQ();
		preserveSpace = reader.ReadBoolean();
	}
}

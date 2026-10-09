using System.Xml;

namespace System.Security.Cryptography.Xml;

public class KeyInfoNode : KeyInfoClause
{
	private XmlElement _node;

	public XmlElement Value
	{
		get
		{
			return _node;
		}
		set
		{
			_node = value;
		}
	}

	public KeyInfoNode()
	{
	}

	public KeyInfoNode(XmlElement node)
	{
		_node = node;
	}

	public override XmlElement GetXml()
	{
		XmlDocument xmlDocument = new XmlDocument();
		xmlDocument.PreserveWhitespace = true;
		return GetXml(xmlDocument);
	}

	internal override XmlElement GetXml(XmlDocument xmlDocument)
	{
		return xmlDocument.ImportNode(_node, deep: true) as XmlElement;
	}

	public override void LoadXml(XmlElement value)
	{
		_node = value;
	}
}

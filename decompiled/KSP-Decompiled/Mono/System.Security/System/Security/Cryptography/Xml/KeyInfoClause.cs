using System.Xml;

namespace System.Security.Cryptography.Xml;

public abstract class KeyInfoClause
{
	public abstract XmlElement GetXml();

	internal virtual XmlElement GetXml(XmlDocument xmlDocument)
	{
		XmlElement xml = GetXml();
		return (XmlElement)xmlDocument.ImportNode(xml, deep: true);
	}

	public abstract void LoadXml(XmlElement element);
}

using System.Collections;
using System.Xml;

namespace System.Security.Cryptography.Xml;

public class KeyInfo : IEnumerable
{
	private string _id;

	private ArrayList _keyInfoClauses;

	public string Id
	{
		get
		{
			return _id;
		}
		set
		{
			_id = value;
		}
	}

	public int Count => _keyInfoClauses.Count;

	public KeyInfo()
	{
		_keyInfoClauses = new ArrayList();
	}

	public XmlElement GetXml()
	{
		XmlDocument xmlDocument = new XmlDocument();
		xmlDocument.PreserveWhitespace = true;
		return GetXml(xmlDocument);
	}

	internal XmlElement GetXml(XmlDocument xmlDocument)
	{
		XmlElement xmlElement = xmlDocument.CreateElement("KeyInfo", "http://www.w3.org/2000/09/xmldsig#");
		if (!string.IsNullOrEmpty(_id))
		{
			xmlElement.SetAttribute("Id", _id);
		}
		for (int i = 0; i < _keyInfoClauses.Count; i++)
		{
			XmlElement xml = ((KeyInfoClause)_keyInfoClauses[i]).GetXml(xmlDocument);
			if (xml != null)
			{
				xmlElement.AppendChild(xml);
			}
		}
		return xmlElement;
	}

	public void LoadXml(XmlElement value)
	{
		if (value == null)
		{
			throw new ArgumentNullException("value");
		}
		_id = Utils.GetAttribute(value, "Id", "http://www.w3.org/2000/09/xmldsig#");
		for (XmlNode xmlNode = value.FirstChild; xmlNode != null; xmlNode = xmlNode.NextSibling)
		{
			if (xmlNode is XmlElement xmlElement)
			{
				string text = xmlElement.NamespaceURI + " " + xmlElement.LocalName;
				if (text == "http://www.w3.org/2000/09/xmldsig# KeyValue")
				{
					foreach (XmlNode childNode in xmlElement.ChildNodes)
					{
						if (childNode is XmlElement xmlElement2)
						{
							text = text + "/" + xmlElement2.LocalName;
							break;
						}
					}
				}
				KeyInfoClause keyInfoClause = (KeyInfoClause)CryptoHelpers.CreateFromName(text);
				if (keyInfoClause == null)
				{
					keyInfoClause = new KeyInfoNode();
				}
				keyInfoClause.LoadXml(xmlElement);
				AddClause(keyInfoClause);
			}
		}
	}

	public void AddClause(KeyInfoClause clause)
	{
		_keyInfoClauses.Add(clause);
	}

	public IEnumerator GetEnumerator()
	{
		return _keyInfoClauses.GetEnumerator();
	}

	public IEnumerator GetEnumerator(Type requestedObjectType)
	{
		ArrayList arrayList = new ArrayList();
		IEnumerator enumerator = _keyInfoClauses.GetEnumerator();
		while (enumerator.MoveNext())
		{
			object current = enumerator.Current;
			if (requestedObjectType.Equals(current.GetType()))
			{
				arrayList.Add(current);
			}
		}
		return arrayList.GetEnumerator();
	}
}

using System.Collections;

namespace System.Xml.Serialization;

public class XmlAttributeOverrides
{
	private Hashtable types = new Hashtable();

	public XmlAttributes this[Type type] => this[type, string.Empty];

	public XmlAttributes this[Type type, string member]
	{
		get
		{
			Hashtable hashtable = (Hashtable)types[type];
			if (hashtable == null)
			{
				return null;
			}
			return (XmlAttributes)hashtable[member];
		}
	}

	public void Add(Type type, XmlAttributes attributes)
	{
		Add(type, string.Empty, attributes);
	}

	public void Add(Type type, string member, XmlAttributes attributes)
	{
		Hashtable hashtable = (Hashtable)types[type];
		if (hashtable == null)
		{
			hashtable = new Hashtable();
			types.Add(type, hashtable);
		}
		else if (hashtable[member] != null)
		{
			throw new InvalidOperationException(Res.GetString("'{0}.{1}' already has attributes.", type.FullName, member));
		}
		hashtable.Add(member, attributes);
	}
}

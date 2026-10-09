using System.Collections;

namespace System.Xml.Serialization;

public class SoapAttributeOverrides
{
	private Hashtable types = new Hashtable();

	public SoapAttributes this[Type type] => this[type, string.Empty];

	public SoapAttributes this[Type type, string member]
	{
		get
		{
			Hashtable hashtable = (Hashtable)types[type];
			if (hashtable == null)
			{
				return null;
			}
			return (SoapAttributes)hashtable[member];
		}
	}

	public void Add(Type type, SoapAttributes attributes)
	{
		Add(type, string.Empty, attributes);
	}

	public void Add(Type type, string member, SoapAttributes attributes)
	{
		Hashtable hashtable = (Hashtable)types[type];
		if (hashtable == null)
		{
			hashtable = new Hashtable();
			types.Add(type, hashtable);
		}
		else if (hashtable[member] != null)
		{
			throw new InvalidOperationException(Res.GetString("{0}. {1} already has attributes.", type.FullName, member));
		}
		hashtable.Add(member, attributes);
	}
}

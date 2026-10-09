using System.Collections;

namespace System.Xml.Serialization;

internal class NameTable : INameScope
{
	private Hashtable table = new Hashtable();

	internal object this[XmlQualifiedName qname]
	{
		get
		{
			return table[new NameKey(qname.Name, qname.Namespace)];
		}
		set
		{
			table[new NameKey(qname.Name, qname.Namespace)] = value;
		}
	}

	internal object this[string name, string ns]
	{
		get
		{
			return table[new NameKey(name, ns)];
		}
		set
		{
			table[new NameKey(name, ns)] = value;
		}
	}

	object INameScope.this[string name, string ns]
	{
		get
		{
			return table[new NameKey(name, ns)];
		}
		set
		{
			table[new NameKey(name, ns)] = value;
		}
	}

	internal ICollection Values => table.Values;

	internal void Add(XmlQualifiedName qname, object value)
	{
		Add(qname.Name, qname.Namespace, value);
	}

	internal void Add(string name, string ns, object value)
	{
		NameKey key = new NameKey(name, ns);
		table.Add(key, value);
	}

	internal Array ToArray(Type type)
	{
		Array array = Array.CreateInstance(type, table.Count);
		table.Values.CopyTo(array, 0);
		return array;
	}
}

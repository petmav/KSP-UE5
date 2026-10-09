using System.Runtime.Serialization;
using System.Security.Permissions;
using Unity;

namespace System.Xml.Linq;

[Serializable]
[KnownType(typeof(NameSerializer))]
public sealed class XName : IEquatable<XName>, ISerializable
{
	private XNamespace ns;

	private string localName;

	private int hashCode;

	public string LocalName => localName;

	public XNamespace Namespace => ns;

	public string NamespaceName => ns.NamespaceName;

	internal XName(XNamespace ns, string localName)
	{
		this.ns = ns;
		this.localName = XmlConvert.VerifyNCName(localName);
		hashCode = ns.GetHashCode() ^ localName.GetHashCode();
	}

	public override string ToString()
	{
		if (ns.NamespaceName.Length == 0)
		{
			return localName;
		}
		return "{" + ns.NamespaceName + "}" + localName;
	}

	public static XName Get(string expandedName)
	{
		if (expandedName == null)
		{
			throw new ArgumentNullException("expandedName");
		}
		if (expandedName.Length == 0)
		{
			throw new ArgumentException(Res.GetString("Argument_InvalidExpandedName", expandedName));
		}
		if (expandedName[0] == '{')
		{
			int num = expandedName.LastIndexOf('}');
			if (num <= 1 || num == expandedName.Length - 1)
			{
				throw new ArgumentException(Res.GetString("Argument_InvalidExpandedName", expandedName));
			}
			return XNamespace.Get(expandedName, 1, num - 1).GetName(expandedName, num + 1, expandedName.Length - num - 1);
		}
		return XNamespace.None.GetName(expandedName);
	}

	public static XName Get(string localName, string namespaceName)
	{
		return XNamespace.Get(namespaceName).GetName(localName);
	}

	[CLSCompliant(false)]
	public static implicit operator XName(string expandedName)
	{
		if (expandedName == null)
		{
			return null;
		}
		return Get(expandedName);
	}

	public override bool Equals(object obj)
	{
		return this == obj;
	}

	public override int GetHashCode()
	{
		return hashCode;
	}

	public static bool operator ==(XName left, XName right)
	{
		return (object)left == right;
	}

	public static bool operator !=(XName left, XName right)
	{
		return (object)left != right;
	}

	bool IEquatable<XName>.Equals(XName other)
	{
		return (object)this == other;
	}

	[SecurityPermission(SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.SerializationFormatter)]
	void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context)
	{
		if (info == null)
		{
			throw new ArgumentNullException("info");
		}
		info.AddValue("name", ToString());
		info.SetType(typeof(NameSerializer));
	}

	internal XName()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

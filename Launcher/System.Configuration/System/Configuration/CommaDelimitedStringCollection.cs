using System.Collections.Specialized;
using System.Reflection;
using Unity;

namespace System.Configuration;

[DefaultMember("Item")]
public sealed class CommaDelimitedStringCollection : StringCollection
{
	public bool IsModified
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public CommaDelimitedStringCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CommaDelimitedStringCollection Clone()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void SetReadOnly()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

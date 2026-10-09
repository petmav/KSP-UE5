using System.Collections.Specialized;
using Unity;

namespace System.Configuration;

[Serializable]
public sealed class PropertyInformationCollection : NameObjectCollectionBase
{
	public PropertyInformation this[string propertyName]
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal PropertyInformationCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void CopyTo(PropertyInformation[] array, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

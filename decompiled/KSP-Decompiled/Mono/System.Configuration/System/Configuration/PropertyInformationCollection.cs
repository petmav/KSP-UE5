using System.Collections;
using System.Collections.Specialized;
using System.Runtime.Serialization;
using System.Security.Permissions;

namespace System.Configuration;

[Serializable]
public sealed class PropertyInformationCollection : NameObjectCollectionBase
{
	private class PropertyInformationEnumerator : IEnumerator
	{
		private PropertyInformationCollection collection;

		private int position;

		public object Current
		{
			get
			{
				if (position < collection.Count && position >= 0)
				{
					return collection.BaseGet(position);
				}
				throw new InvalidOperationException();
			}
		}

		public PropertyInformationEnumerator(PropertyInformationCollection collection)
		{
			this.collection = collection;
			position = -1;
		}

		public bool MoveNext()
		{
			if (++position >= collection.Count)
			{
				return false;
			}
			return true;
		}

		public void Reset()
		{
			position = -1;
		}
	}

	public PropertyInformation this[string propertyName] => (PropertyInformation)BaseGet(propertyName);

	internal PropertyInformationCollection()
		: base(StringComparer.Ordinal)
	{
	}

	public void CopyTo(PropertyInformation[] array, int index)
	{
		((ICollection)this).CopyTo((Array)array, index);
	}

	public override IEnumerator GetEnumerator()
	{
		return new PropertyInformationEnumerator(this);
	}

	internal void Add(PropertyInformation pi)
	{
		BaseAdd(pi.Name, pi);
	}

	[System.MonoTODO]
	[SecurityPermission(SecurityAction.Demand, SerializationFormatter = true)]
	public override void GetObjectData(SerializationInfo info, StreamingContext context)
	{
		throw new NotImplementedException();
	}
}

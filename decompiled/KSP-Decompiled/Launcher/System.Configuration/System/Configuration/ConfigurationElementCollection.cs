using System.Collections;
using System.Diagnostics;
using System.Xml;
using Unity;

namespace System.Configuration;

[DebuggerDisplay("Count = {Count}")]
public abstract class ConfigurationElementCollection : ConfigurationElement, ICollection, IEnumerable
{
	protected internal string AddElementName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	protected internal string ClearElementName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public virtual ConfigurationElementCollectionType CollectionType
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(ConfigurationElementCollectionType);
		}
	}

	public int Count
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	protected virtual string ElementName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public bool EmitClear
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public bool IsSynchronized
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	protected internal string RemoveElementName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public object SyncRoot
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	protected virtual bool ThrowOnDuplicate
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	protected ConfigurationElementCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected ConfigurationElementCollection(IComparer comparer)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void BaseAdd(ConfigurationElement element)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal void BaseAdd(ConfigurationElement element, bool throwIfExists)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void BaseAdd(int index, ConfigurationElement element)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal void BaseClear()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal ConfigurationElement BaseGet(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected internal ConfigurationElement BaseGet(object key)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected internal object[] BaseGetAllKeys()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected internal object BaseGetKey(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected int BaseIndexOf(ConfigurationElement element)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	protected internal bool BaseIsRemoved(object key)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	protected internal void BaseRemove(object key)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal void BaseRemoveAt(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void CopyTo(ConfigurationElement[] array, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected abstract ConfigurationElement CreateNewElement();

	protected virtual ConfigurationElement CreateNewElement(string elementName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected abstract object GetElementKey(ConfigurationElement element);

	public IEnumerator GetEnumerator()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected virtual bool IsElementName(string elementName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	protected virtual bool IsElementRemovable(ConfigurationElement element)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	protected internal override bool IsModified()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public override bool IsReadOnly()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	protected override bool OnDeserializeUnrecognizedElement(string elementName, XmlReader reader)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	protected internal override void Reset(ConfigurationElement parentElement)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal override void ResetModified()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal override bool SerializeElement(XmlWriter writer, bool serializeCollectionKey)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	protected internal override void SetReadOnly()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	void ICollection.CopyTo(Array arr, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected internal override void Unmerge(ConfigurationElement sourceElement, ConfigurationElement parentElement, ConfigurationSaveMode saveMode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

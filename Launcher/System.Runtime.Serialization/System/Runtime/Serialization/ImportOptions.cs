using System.CodeDom.Compiler;
using System.Collections.Generic;
using Unity;

namespace System.Runtime.Serialization;

public class ImportOptions
{
	public CodeDomProvider CodeProvider
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

	public IDataContractSurrogate DataContractSurrogate
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

	public bool EnableDataBinding
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

	public bool GenerateInternal
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

	public bool GenerateSerializable
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

	public bool ImportXmlType
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

	public IDictionary<string, string> Namespaces
	{
		get
		{
			//IL_0007: Expected O, but got I4
			Unity.ThrowStub.ThrowNotSupportedException();
			return (IDictionary<string, string>)0;
		}
	}

	public ICollection<Type> ReferencedCollectionTypes
	{
		get
		{
			//IL_0007: Expected O, but got I4
			Unity.ThrowStub.ThrowNotSupportedException();
			return (ICollection<Type>)0;
		}
	}

	public ICollection<Type> ReferencedTypes
	{
		get
		{
			//IL_0007: Expected O, but got I4
			Unity.ThrowStub.ThrowNotSupportedException();
			return (ICollection<Type>)0;
		}
	}

	public ImportOptions()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

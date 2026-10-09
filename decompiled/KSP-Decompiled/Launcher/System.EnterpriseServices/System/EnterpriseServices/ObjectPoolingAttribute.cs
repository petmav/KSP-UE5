using System.Collections;
using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices;

[AttributeUsage(AttributeTargets.Class, Inherited = true)]
[ComVisible(false)]
public sealed class ObjectPoolingAttribute : Attribute
{
	public int CreationTimeout
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public bool Enabled
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

	public int MaxPoolSize
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public int MinPoolSize
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public ObjectPoolingAttribute()
	{
	}

	public ObjectPoolingAttribute(bool enable)
	{
	}

	public ObjectPoolingAttribute(bool enable, int minPoolSize, int maxPoolSize)
	{
	}

	public ObjectPoolingAttribute(int minPoolSize, int maxPoolSize)
	{
	}

	public bool AfterSaveChanges(Hashtable info)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool Apply(Hashtable info)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsValidTarget(string s)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}
}

using System.Collections;
using System.ComponentModel;
using System.Data.Common;
using System.Runtime.CompilerServices;
using Unity;

namespace System.Data.Odbc;

[Editor("Microsoft.VSDesigner.Data.Design.DBParametersEditor, Microsoft.VSDesigner, Version=10.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a", "System.Drawing.Design.UITypeEditor, System.Drawing, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a")]
[ListBindable(false)]
public sealed class OdbcParameterCollection : DbParameterCollection
{
	public override int Count
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public new string this[string parameterName]
	{
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public override object SyncRoot
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal OdbcParameterCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SpecialName]
	public void set_Item(int index, OdbcParameter value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OdbcParameter Add(OdbcParameter value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[EditorBrowsable(EditorBrowsableState.Never)]
	public override int Add(object value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public OdbcParameter Add(string parameterName, OdbcType odbcType)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public OdbcParameter Add(string parameterName, OdbcType odbcType, int size)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public OdbcParameter Add(string parameterName, OdbcType odbcType, int size, string sourceColumn)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[EditorBrowsable(EditorBrowsableState.Never)]
	[Obsolete("Add(String parameterName, Object value) has been deprecated.  Use AddWithValue(String parameterName, Object value).  http://go.microsoft.com/fwlink/?linkid=14202", false)]
	public OdbcParameter Add(string parameterName, object value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override void AddRange(Array values)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddRange(OdbcParameter[] values)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OdbcParameter AddWithValue(string parameterName, object value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override void Clear()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public bool Contains(OdbcParameter value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public override bool Contains(object value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public override bool Contains(string value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public override void CopyTo(Array array, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void CopyTo(OdbcParameter[] array, int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override IEnumerator GetEnumerator()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected override DbParameter GetParameter(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected override DbParameter GetParameter(string parameterName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public int IndexOf(OdbcParameter value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public override int IndexOf(object value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public override int IndexOf(string parameterName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void Insert(int index, OdbcParameter value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override void Insert(int index, object value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Remove(OdbcParameter value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override void Remove(object value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override void RemoveAt(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override void RemoveAt(string parameterName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override void SetParameter(int index, DbParameter value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override void SetParameter(string parameterName, DbParameter value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

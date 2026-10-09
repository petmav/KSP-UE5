using System.ComponentModel;
using System.Data.Common;
using Unity;

namespace System.Data.OleDb;

[TypeConverter("System.Data.OleDb.OleDbParameter.OleDbParameterConverter")]
public sealed class OleDbParameter : DbParameter, ICloneable
{
	public override DbType DbType
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(DbType);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public override ParameterDirection Direction
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(ParameterDirection);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public override bool IsNullable
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

	public OleDbType OleDbType
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(OleDbType);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public override string ParameterName
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

	public override int Size
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

	public override string SourceColumn
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

	public override bool SourceColumnNullMapping
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

	public override object Value
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

	public OleDbParameter()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OleDbParameter(string name, OleDbType dataType)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OleDbParameter(string name, OleDbType dataType, int size)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[EditorBrowsable(EditorBrowsableState.Advanced)]
	public OleDbParameter(string parameterName, OleDbType dbType, int size, ParameterDirection direction, bool isNullable, byte precision, byte scale, string srcColumn, DataRowVersion srcVersion, object value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[EditorBrowsable(EditorBrowsableState.Advanced)]
	public OleDbParameter(string parameterName, OleDbType dbType, int size, ParameterDirection direction, byte precision, byte scale, string sourceColumn, DataRowVersion sourceVersion, bool sourceColumnNullMapping, object value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OleDbParameter(string name, OleDbType dataType, int size, string srcColumn)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OleDbParameter(string name, object value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override void ResetDbType()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ResetOleDbType()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	object ICloneable.Clone()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

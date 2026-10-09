using System.ComponentModel;
using System.Data.Common;
using Unity;

namespace System.Data.Odbc;

[TypeConverter("System.Data.Odbc.OdbcParameter.OdbcParameterConverter")]
public sealed class OdbcParameter : DbParameter, ICloneable
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

	public OdbcType OdbcType
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(OdbcType);
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

	public OdbcParameter()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OdbcParameter(string name, OdbcType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OdbcParameter(string name, OdbcType type, int size)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[EditorBrowsable(EditorBrowsableState.Advanced)]
	public OdbcParameter(string parameterName, OdbcType odbcType, int size, ParameterDirection parameterDirection, bool isNullable, byte precision, byte scale, string srcColumn, DataRowVersion srcVersion, object value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[EditorBrowsable(EditorBrowsableState.Advanced)]
	public OdbcParameter(string parameterName, OdbcType odbcType, int size, ParameterDirection parameterDirection, byte precision, byte scale, string sourceColumn, DataRowVersion sourceVersion, bool sourceColumnNullMapping, object value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OdbcParameter(string name, OdbcType type, int size, string sourcecolumn)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OdbcParameter(string name, object value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override void ResetDbType()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ResetOdbcType()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	object ICloneable.Clone()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}

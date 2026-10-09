using System.Collections;
using System.ComponentModel;
using System.Data.Common;
using System.Runtime.CompilerServices;
using Unity;

namespace System.Data.OleDb;

public sealed class OleDbDataReader : DbDataReader
{
	public override int Depth
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public override int FieldCount
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public override bool HasRows
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public override bool IsClosed
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public override object this[string name]
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public override int RecordsAffected
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	internal OleDbDataReader()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SpecialName]
	public object get_Item(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override bool GetBoolean(int ordinal)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public override byte GetByte(int ordinal)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(byte);
	}

	public override long GetBytes(int ordinal, long dataIndex, byte[] buffer, int bufferIndex, int length)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(long);
	}

	[EditorBrowsable(EditorBrowsableState.Never)]
	public override char GetChar(int ordinal)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(char);
	}

	public override long GetChars(int ordinal, long dataIndex, char[] buffer, int bufferIndex, int length)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(long);
	}

	public override string GetDataTypeName(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override DateTime GetDateTime(int ordinal)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(DateTime);
	}

	public override decimal GetDecimal(int ordinal)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(decimal);
	}

	public override double GetDouble(int ordinal)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(double);
	}

	public override IEnumerator GetEnumerator()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override Type GetFieldType(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override float GetFloat(int ordinal)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(float);
	}

	public override Guid GetGuid(int ordinal)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(Guid);
	}

	public override short GetInt16(int ordinal)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(short);
	}

	public override int GetInt32(int ordinal)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public override long GetInt64(int ordinal)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(long);
	}

	public override string GetName(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override int GetOrdinal(string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public override string GetString(int ordinal)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public TimeSpan GetTimeSpan(int ordinal)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(TimeSpan);
	}

	public override object GetValue(int ordinal)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override int GetValues(object[] values)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public override bool IsDBNull(int ordinal)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public override bool NextResult()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public override bool Read()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}
}

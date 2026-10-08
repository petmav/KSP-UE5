using System.Collections;
using System.Data.Common;
using System.Runtime.CompilerServices;
using Unity;

namespace System.Data.Odbc;

public sealed class OdbcDataReader : DbDataReader
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

	public override object this[string value]
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

	internal OdbcDataReader()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SpecialName]
	public object get_Item(int i)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override bool GetBoolean(int i)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public override byte GetByte(int i)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(byte);
	}

	public override long GetBytes(int i, long dataIndex, byte[] buffer, int bufferIndex, int length)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(long);
	}

	public override char GetChar(int i)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(char);
	}

	public override long GetChars(int i, long dataIndex, char[] buffer, int bufferIndex, int length)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(long);
	}

	public override string GetDataTypeName(int i)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public DateTime GetDate(int i)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(DateTime);
	}

	public override DateTime GetDateTime(int i)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(DateTime);
	}

	public override decimal GetDecimal(int i)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(decimal);
	}

	public override double GetDouble(int i)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(double);
	}

	public override IEnumerator GetEnumerator()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override Type GetFieldType(int i)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override float GetFloat(int i)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(float);
	}

	public override Guid GetGuid(int i)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(Guid);
	}

	public override short GetInt16(int i)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(short);
	}

	public override int GetInt32(int i)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public override long GetInt64(int i)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(long);
	}

	public override string GetName(int i)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override int GetOrdinal(string value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public override string GetString(int i)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public TimeSpan GetTime(int i)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(TimeSpan);
	}

	public override object GetValue(int i)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override int GetValues(object[] values)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public override bool IsDBNull(int i)
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

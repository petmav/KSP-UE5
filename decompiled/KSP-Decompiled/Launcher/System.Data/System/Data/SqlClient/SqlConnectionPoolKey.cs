using System.Data.Common;

namespace System.Data.SqlClient;

internal class SqlConnectionPoolKey : DbConnectionPoolKey
{
	private int _hashValue;

	internal override string ConnectionString
	{
		get
		{
			return base.ConnectionString;
		}
		set
		{
			base.ConnectionString = value;
			CalculateHashCode();
		}
	}

	internal SqlConnectionPoolKey(string connectionString)
		: base(connectionString)
	{
		CalculateHashCode();
	}

	private SqlConnectionPoolKey(SqlConnectionPoolKey key)
		: base(key)
	{
		CalculateHashCode();
	}

	public override object Clone()
	{
		return new SqlConnectionPoolKey(this);
	}

	public override bool Equals(object obj)
	{
		if (obj is SqlConnectionPoolKey sqlConnectionPoolKey)
		{
			return ConnectionString == sqlConnectionPoolKey.ConnectionString;
		}
		return false;
	}

	public override int GetHashCode()
	{
		return _hashValue;
	}

	private void CalculateHashCode()
	{
		_hashValue = base.GetHashCode();
	}
}

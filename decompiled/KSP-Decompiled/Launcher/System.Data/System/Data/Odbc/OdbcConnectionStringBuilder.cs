using System.ComponentModel;
using System.Data.Common;
using System.Reflection;
using Unity;

namespace System.Data.Odbc;

[DefaultMember("Item")]
[TypeConverter("System.Data.Odbc.OdbcConnectionStringBuilder.OdbcConnectionStringBuilderConverter")]
[DefaultProperty("Driver")]
public sealed class OdbcConnectionStringBuilder : DbConnectionStringBuilder
{
	public string Driver
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

	public string Dsn
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

	public OdbcConnectionStringBuilder()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OdbcConnectionStringBuilder(string connectionString)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

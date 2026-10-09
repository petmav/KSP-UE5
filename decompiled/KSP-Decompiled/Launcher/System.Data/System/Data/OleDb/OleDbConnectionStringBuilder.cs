using System.ComponentModel;
using System.Data.Common;
using System.Reflection;
using Unity;

namespace System.Data.OleDb;

[DefaultMember("Item")]
[DefaultProperty("Provider")]
[RefreshProperties(RefreshProperties.All)]
[TypeConverter("System.Data.OleDb.OleDbConnectionStringBuilder.OleDbConnectionStringBuilderConverter")]
public sealed class OleDbConnectionStringBuilder : DbConnectionStringBuilder
{
	public string DataSource
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

	public string FileName
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

	public int OleDbServices
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

	public bool PersistSecurityInfo
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

	public string Provider
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

	public OleDbConnectionStringBuilder()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OleDbConnectionStringBuilder(string connectionString)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

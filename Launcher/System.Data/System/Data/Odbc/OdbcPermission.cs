using System.Data.Common;
using System.Security.Permissions;
using Unity;

namespace System.Data.Odbc;

[Serializable]
public sealed class OdbcPermission : DBDataPermission
{
	[Obsolete("OdbcPermission() has been deprecated.  Use the OdbcPermission(PermissionState.None) constructor.  http://go.microsoft.com/fwlink/?linkid=14202", true)]
	public OdbcPermission()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OdbcPermission(PermissionState state)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[Obsolete("OdbcPermission(PermissionState state, Boolean allowBlankPassword) has been deprecated.  Use the OdbcPermission(PermissionState.None) constructor.  http://go.microsoft.com/fwlink/?linkid=14202", true)]
	public OdbcPermission(PermissionState state, bool allowBlankPassword)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

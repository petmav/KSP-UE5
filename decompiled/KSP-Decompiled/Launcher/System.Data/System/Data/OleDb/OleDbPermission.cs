using System.Data.Common;
using System.Security.Permissions;
using Unity;

namespace System.Data.OleDb;

[Serializable]
public sealed class OleDbPermission : DBDataPermission
{
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

	[Obsolete("OleDbPermission() has been deprecated.  Use the OleDbPermission(PermissionState.None) constructor.  http://go.microsoft.com/fwlink/?linkid=14202", true)]
	public OleDbPermission()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public OleDbPermission(PermissionState state)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[Obsolete("OleDbPermission(PermissionState state, Boolean allowBlankPassword) has been deprecated.  Use the OleDbPermission(PermissionState.None) constructor.  http://go.microsoft.com/fwlink/?linkid=14202", true)]
	public OleDbPermission(PermissionState state, bool allowBlankPassword)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

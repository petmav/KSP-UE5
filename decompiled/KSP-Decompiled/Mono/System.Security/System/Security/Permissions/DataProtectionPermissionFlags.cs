namespace System.Security.Permissions;

[Serializable]
[Flags]
public enum DataProtectionPermissionFlags
{
	NoFlags = 0,
	ProtectData = 1,
	UnprotectData = 2,
	ProtectMemory = 4,
	UnprotectMemory = 8,
	AllFlags = 0xF
}

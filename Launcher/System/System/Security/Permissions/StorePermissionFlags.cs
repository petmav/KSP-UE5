namespace System.Security.Permissions;

[Serializable]
[Flags]
public enum StorePermissionFlags
{
	AddToStore = 0x20,
	AllFlags = 0xF7,
	CreateStore = 1,
	DeleteStore = 2,
	EnumerateCertificates = 0x80,
	EnumerateStores = 4,
	NoFlags = 0,
	OpenStore = 0x10,
	RemoveFromStore = 0x40
}

using System.Runtime.InteropServices;

namespace System.Security.Permissions;

[Serializable]
[ComVisible(true)]
[Flags]
public enum FileDialogPermissionAccess
{
	None = 0,
	Open = 1,
	Save = 2,
	OpenSave = 3
}

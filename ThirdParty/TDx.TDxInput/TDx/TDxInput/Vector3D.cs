using System.Runtime.InteropServices;

namespace TDx.TDxInput
{
	[ComImport]
	[Guid("8C2AA71D-2B23-43F5-A6ED-4DF57E9CD8D5")]
	[CoClass(typeof(Vector3DClass))]
	public interface Vector3D : IVector3D
	{
	}
}

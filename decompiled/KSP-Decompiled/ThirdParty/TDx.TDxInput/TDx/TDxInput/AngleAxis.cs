using System.Runtime.InteropServices;

namespace TDx.TDxInput
{
	[ComImport]
	[CoClass(typeof(AngleAxisClass))]
	[Guid("1EF2BAFF-54E9-4706-9F61-078F7134FD35")]
	public interface AngleAxis : IAngleAxis
	{
	}
}

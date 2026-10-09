using System.Runtime.InteropServices;

namespace TDx.TDxInput
{
	[ComImport]
	[CoClass(typeof(SensorClass))]
	[Guid("F3A6775E-6FA1-4829-BF32-5B045C29078F")]
	public interface Sensor : ISensor, _ISensorEvents_Event
	{
	}
}

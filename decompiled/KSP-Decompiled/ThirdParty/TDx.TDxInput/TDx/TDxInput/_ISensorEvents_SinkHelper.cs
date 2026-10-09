using System.Runtime.InteropServices;

namespace TDx.TDxInput
{
	[TypeLibType(TypeLibTypeFlags.FHidden)]
	[ClassInterface(ClassInterfaceType.None)]
	public sealed class _ISensorEvents_SinkHelper : _ISensorEvents
	{
		public _ISensorEvents_SensorInputEventHandler m_SensorInputDelegate;

		public int m_dwCookie;

		public void SensorInput()
		{
			//Error decoding local variables: Signature type sequence must have at least one element.
			if (m_SensorInputDelegate != null)
			{
				m_SensorInputDelegate();
			}
		}

		internal _ISensorEvents_SinkHelper()
		{
			//Error decoding local variables: Signature type sequence must have at least one element.
			m_dwCookie = 0;
			m_SensorInputDelegate = null;
		}
	}
}

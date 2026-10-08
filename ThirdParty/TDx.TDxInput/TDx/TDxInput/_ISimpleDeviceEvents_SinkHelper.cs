using System.Runtime.InteropServices;

namespace TDx.TDxInput
{
	[ClassInterface(ClassInterfaceType.None)]
	[TypeLibType(TypeLibTypeFlags.FHidden)]
	public sealed class _ISimpleDeviceEvents_SinkHelper : _ISimpleDeviceEvents
	{
		public _ISimpleDeviceEvents_DeviceChangeEventHandler m_DeviceChangeDelegate;

		public int m_dwCookie;

		public void DeviceChange(int P_0)
		{
			//Error decoding local variables: Signature type sequence must have at least one element.
			if (m_DeviceChangeDelegate != null)
			{
				m_DeviceChangeDelegate(P_0);
			}
		}

		internal _ISimpleDeviceEvents_SinkHelper()
		{
			//Error decoding local variables: Signature type sequence must have at least one element.
			m_dwCookie = 0;
			m_DeviceChangeDelegate = null;
		}
	}
}

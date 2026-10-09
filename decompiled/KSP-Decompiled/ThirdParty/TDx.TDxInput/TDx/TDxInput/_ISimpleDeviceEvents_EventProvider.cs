using System;
using System.Collections;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
using System.Threading;

namespace TDx.TDxInput
{
	internal sealed class _ISimpleDeviceEvents_EventProvider : _ISimpleDeviceEvents_Event, IDisposable
	{
		private IConnectionPointContainer m_ConnectionPointContainer;

		private ArrayList m_aEventSinkHelpers;

		private IConnectionPoint m_ConnectionPoint;

		private void Init()
		{
			IConnectionPoint ppCP = null;
			Guid riid = new Guid(new byte[16]
			{
				22, 162, 227, 143, 53, 226, 166, 73, 145, 54,
				249, 216, 31, 218, 222, 245
			});
			m_ConnectionPointContainer.FindConnectionPoint(ref riid, out ppCP);
			m_ConnectionPoint = ppCP;
			m_aEventSinkHelpers = new ArrayList();
		}

		public void add_DeviceChange(_ISimpleDeviceEvents_DeviceChangeEventHandler P_0)
		{
			Monitor.Enter(this);
			try
			{
				if (m_ConnectionPoint == null)
				{
					Init();
				}
				_ISimpleDeviceEvents_SinkHelper iSimpleDeviceEvents_SinkHelper = new _ISimpleDeviceEvents_SinkHelper();
				int pdwCookie = 0;
				m_ConnectionPoint.Advise(iSimpleDeviceEvents_SinkHelper, out pdwCookie);
				iSimpleDeviceEvents_SinkHelper.m_dwCookie = pdwCookie;
				iSimpleDeviceEvents_SinkHelper.m_DeviceChangeDelegate = P_0;
				m_aEventSinkHelpers.Add(iSimpleDeviceEvents_SinkHelper);
			}
			finally
			{
				Monitor.Exit(this);
			}
		}

		public void remove_DeviceChange(_ISimpleDeviceEvents_DeviceChangeEventHandler P_0)
		{
			Monitor.Enter(this);
			try
			{
				if (m_aEventSinkHelpers == null)
				{
					return;
				}
				int count = m_aEventSinkHelpers.Count;
				int num = 0;
				if (0 >= count)
				{
					return;
				}
				do
				{
					_ISimpleDeviceEvents_SinkHelper iSimpleDeviceEvents_SinkHelper = (_ISimpleDeviceEvents_SinkHelper)m_aEventSinkHelpers[num];
					if (iSimpleDeviceEvents_SinkHelper.m_DeviceChangeDelegate != null && ((iSimpleDeviceEvents_SinkHelper.m_DeviceChangeDelegate.Equals(P_0) ? 1u : 0u) & 0xFFu) != 0)
					{
						m_aEventSinkHelpers.RemoveAt(num);
						m_ConnectionPoint.Unadvise(iSimpleDeviceEvents_SinkHelper.m_dwCookie);
						if (count <= 1)
						{
							Marshal.ReleaseComObject(m_ConnectionPoint);
							m_ConnectionPoint = null;
							m_aEventSinkHelpers = null;
						}
						break;
					}
					num++;
				}
				while (num < count);
			}
			finally
			{
				Monitor.Exit(this);
			}
		}

		public _ISimpleDeviceEvents_EventProvider(object P_0)
		{
			//Error decoding local variables: Signature type sequence must have at least one element.
			m_ConnectionPointContainer = (IConnectionPointContainer)P_0;
		}

		public void Finalize()
		{
			Monitor.Enter(this);
			try
			{
				if (m_ConnectionPoint == null)
				{
					return;
				}
				int count = m_aEventSinkHelpers.Count;
				int num = 0;
				if (0 < count)
				{
					do
					{
						_ISimpleDeviceEvents_SinkHelper iSimpleDeviceEvents_SinkHelper = (_ISimpleDeviceEvents_SinkHelper)m_aEventSinkHelpers[num];
						m_ConnectionPoint.Unadvise(iSimpleDeviceEvents_SinkHelper.m_dwCookie);
						num++;
					}
					while (num < count);
				}
				Marshal.ReleaseComObject(m_ConnectionPoint);
			}
			catch (Exception)
			{
			}
			finally
			{
				Monitor.Exit(this);
			}
		}

		public void Dispose()
		{
			//Error decoding local variables: Signature type sequence must have at least one element.
			Finalize();
			GC.SuppressFinalize(this);
		}
	}
}

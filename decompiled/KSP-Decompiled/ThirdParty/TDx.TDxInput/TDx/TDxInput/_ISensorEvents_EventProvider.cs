using System;
using System.Collections;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
using System.Threading;

namespace TDx.TDxInput
{
	internal sealed class _ISensorEvents_EventProvider : _ISensorEvents_Event, IDisposable
	{
		private IConnectionPointContainer m_ConnectionPointContainer;

		private ArrayList m_aEventSinkHelpers;

		private IConnectionPoint m_ConnectionPoint;

		private void Init()
		{
			IConnectionPoint ppCP = null;
			Guid riid = new Guid(new byte[16]
			{
				74, 154, 146, 230, 65, 111, 198, 70, 146, 82,
				168, 204, 83, 71, 44, 177
			});
			m_ConnectionPointContainer.FindConnectionPoint(ref riid, out ppCP);
			m_ConnectionPoint = ppCP;
			m_aEventSinkHelpers = new ArrayList();
		}

		public void add_SensorInput(_ISensorEvents_SensorInputEventHandler P_0)
		{
			Monitor.Enter(this);
			try
			{
				if (m_ConnectionPoint == null)
				{
					Init();
				}
				_ISensorEvents_SinkHelper iSensorEvents_SinkHelper = new _ISensorEvents_SinkHelper();
				int pdwCookie = 0;
				m_ConnectionPoint.Advise(iSensorEvents_SinkHelper, out pdwCookie);
				iSensorEvents_SinkHelper.m_dwCookie = pdwCookie;
				iSensorEvents_SinkHelper.m_SensorInputDelegate = P_0;
				m_aEventSinkHelpers.Add(iSensorEvents_SinkHelper);
			}
			finally
			{
				Monitor.Exit(this);
			}
		}

		public void remove_SensorInput(_ISensorEvents_SensorInputEventHandler P_0)
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
					_ISensorEvents_SinkHelper iSensorEvents_SinkHelper = (_ISensorEvents_SinkHelper)m_aEventSinkHelpers[num];
					if (iSensorEvents_SinkHelper.m_SensorInputDelegate != null && ((iSensorEvents_SinkHelper.m_SensorInputDelegate.Equals(P_0) ? 1u : 0u) & 0xFFu) != 0)
					{
						m_aEventSinkHelpers.RemoveAt(num);
						m_ConnectionPoint.Unadvise(iSensorEvents_SinkHelper.m_dwCookie);
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

		public _ISensorEvents_EventProvider(object P_0)
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
						_ISensorEvents_SinkHelper iSensorEvents_SinkHelper = (_ISensorEvents_SinkHelper)m_aEventSinkHelpers[num];
						m_ConnectionPoint.Unadvise(iSensorEvents_SinkHelper.m_dwCookie);
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

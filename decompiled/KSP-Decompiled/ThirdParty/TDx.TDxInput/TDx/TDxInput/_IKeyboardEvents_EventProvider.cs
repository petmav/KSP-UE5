using System;
using System.Collections;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
using System.Threading;

namespace TDx.TDxInput
{
	internal sealed class _IKeyboardEvents_EventProvider : _IKeyboardEvents_Event, IDisposable
	{
		private IConnectionPointContainer m_ConnectionPointContainer;

		private ArrayList m_aEventSinkHelpers;

		private IConnectionPoint m_ConnectionPoint;

		private void Init()
		{
			IConnectionPoint ppCP = null;
			Guid riid = new Guid(new byte[16]
			{
				168, 176, 107, 107, 145, 68, 207, 64, 177, 169,
				193, 90, 128, 31, 225, 81
			});
			m_ConnectionPointContainer.FindConnectionPoint(ref riid, out ppCP);
			m_ConnectionPoint = ppCP;
			m_aEventSinkHelpers = new ArrayList();
		}

		public void add_KeyDown(_IKeyboardEvents_KeyDownEventHandler P_0)
		{
			Monitor.Enter(this);
			try
			{
				if (m_ConnectionPoint == null)
				{
					Init();
				}
				_IKeyboardEvents_SinkHelper iKeyboardEvents_SinkHelper = new _IKeyboardEvents_SinkHelper();
				int pdwCookie = 0;
				m_ConnectionPoint.Advise(iKeyboardEvents_SinkHelper, out pdwCookie);
				iKeyboardEvents_SinkHelper.m_dwCookie = pdwCookie;
				iKeyboardEvents_SinkHelper.m_KeyDownDelegate = P_0;
				m_aEventSinkHelpers.Add(iKeyboardEvents_SinkHelper);
			}
			finally
			{
				Monitor.Exit(this);
			}
		}

		public void remove_KeyDown(_IKeyboardEvents_KeyDownEventHandler P_0)
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
					_IKeyboardEvents_SinkHelper iKeyboardEvents_SinkHelper = (_IKeyboardEvents_SinkHelper)m_aEventSinkHelpers[num];
					if (iKeyboardEvents_SinkHelper.m_KeyDownDelegate != null && ((iKeyboardEvents_SinkHelper.m_KeyDownDelegate.Equals(P_0) ? 1u : 0u) & 0xFFu) != 0)
					{
						m_aEventSinkHelpers.RemoveAt(num);
						m_ConnectionPoint.Unadvise(iKeyboardEvents_SinkHelper.m_dwCookie);
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

		public void add_KeyUp(_IKeyboardEvents_KeyUpEventHandler P_0)
		{
			Monitor.Enter(this);
			try
			{
				if (m_ConnectionPoint == null)
				{
					Init();
				}
				_IKeyboardEvents_SinkHelper iKeyboardEvents_SinkHelper = new _IKeyboardEvents_SinkHelper();
				int pdwCookie = 0;
				m_ConnectionPoint.Advise(iKeyboardEvents_SinkHelper, out pdwCookie);
				iKeyboardEvents_SinkHelper.m_dwCookie = pdwCookie;
				iKeyboardEvents_SinkHelper.m_KeyUpDelegate = P_0;
				m_aEventSinkHelpers.Add(iKeyboardEvents_SinkHelper);
			}
			finally
			{
				Monitor.Exit(this);
			}
		}

		public void remove_KeyUp(_IKeyboardEvents_KeyUpEventHandler P_0)
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
					_IKeyboardEvents_SinkHelper iKeyboardEvents_SinkHelper = (_IKeyboardEvents_SinkHelper)m_aEventSinkHelpers[num];
					if (iKeyboardEvents_SinkHelper.m_KeyUpDelegate != null && ((iKeyboardEvents_SinkHelper.m_KeyUpDelegate.Equals(P_0) ? 1u : 0u) & 0xFFu) != 0)
					{
						m_aEventSinkHelpers.RemoveAt(num);
						m_ConnectionPoint.Unadvise(iKeyboardEvents_SinkHelper.m_dwCookie);
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

		public _IKeyboardEvents_EventProvider(object P_0)
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
						_IKeyboardEvents_SinkHelper iKeyboardEvents_SinkHelper = (_IKeyboardEvents_SinkHelper)m_aEventSinkHelpers[num];
						m_ConnectionPoint.Unadvise(iKeyboardEvents_SinkHelper.m_dwCookie);
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

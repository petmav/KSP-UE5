using System.Runtime.InteropServices;

namespace TDx.TDxInput
{
	[TypeLibType(TypeLibTypeFlags.FHidden)]
	[ClassInterface(ClassInterfaceType.None)]
	public sealed class _IKeyboardEvents_SinkHelper : _IKeyboardEvents
	{
		public _IKeyboardEvents_KeyDownEventHandler m_KeyDownDelegate;

		public _IKeyboardEvents_KeyUpEventHandler m_KeyUpDelegate;

		public int m_dwCookie;

		public void KeyDown(int P_0)
		{
			//Error decoding local variables: Signature type sequence must have at least one element.
			if (m_KeyDownDelegate != null)
			{
				m_KeyDownDelegate(P_0);
			}
		}

		public void KeyUp(int P_0)
		{
			//Error decoding local variables: Signature type sequence must have at least one element.
			if (m_KeyUpDelegate != null)
			{
				m_KeyUpDelegate(P_0);
			}
		}

		internal _IKeyboardEvents_SinkHelper()
		{
			//Error decoding local variables: Signature type sequence must have at least one element.
			m_dwCookie = 0;
			m_KeyDownDelegate = null;
			m_KeyUpDelegate = null;
		}
	}
}

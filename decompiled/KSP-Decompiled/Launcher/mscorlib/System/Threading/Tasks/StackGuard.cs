using System.Security;

namespace System.Threading.Tasks;

internal class StackGuard
{
	private int m_inliningDepth;

	private const int MAX_UNCHECKED_INLINING_DEPTH = 20;

	[SecuritySafeCritical]
	internal bool TryBeginInliningScope()
	{
		if (m_inliningDepth < 20 || CheckForSufficientStack())
		{
			m_inliningDepth++;
			return true;
		}
		return false;
	}

	internal void EndInliningScope()
	{
		m_inliningDepth--;
		if (m_inliningDepth < 0)
		{
			m_inliningDepth = 0;
		}
	}

	[SecurityCritical]
	private bool CheckForSufficientStack()
	{
		return true;
	}
}

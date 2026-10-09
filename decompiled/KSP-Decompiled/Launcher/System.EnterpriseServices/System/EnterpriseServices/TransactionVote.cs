using System.Runtime.InteropServices;

namespace System.EnterpriseServices;

[Serializable]
[ComVisible(false)]
public enum TransactionVote
{
	Abort = 1,
	Commit = 0
}

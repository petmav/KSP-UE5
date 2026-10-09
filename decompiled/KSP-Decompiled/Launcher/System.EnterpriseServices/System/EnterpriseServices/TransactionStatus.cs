using System.Runtime.InteropServices;

namespace System.EnterpriseServices;

[Serializable]
[ComVisible(false)]
public enum TransactionStatus
{
	Aborted = 4,
	Aborting = 3,
	Commited = 0,
	LocallyOk = 1,
	NoTransaction = 2
}

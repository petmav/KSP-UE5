namespace System.EnterpriseServices.CompensatingResourceManager;

[Serializable]
public enum TransactionState
{
	Aborted = 2,
	Active = 0,
	Committed = 1,
	Indoubt = 3
}

namespace System.EnterpriseServices;

[Serializable]
public enum TransactionOption
{
	Disabled = 0,
	NotSupported = 1,
	Required = 3,
	RequiresNew = 4,
	Supported = 2
}

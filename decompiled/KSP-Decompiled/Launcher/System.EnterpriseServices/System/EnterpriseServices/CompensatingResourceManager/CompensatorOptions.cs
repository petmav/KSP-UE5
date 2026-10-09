namespace System.EnterpriseServices.CompensatingResourceManager;

[Serializable]
[Flags]
public enum CompensatorOptions
{
	AbortPhase = 4,
	AllPhases = 7,
	CommitPhase = 2,
	FailIfInDoubtsRemain = 0x10,
	PreparePhase = 1
}

namespace System.Diagnostics;

public enum TraceLogRetentionOption
{
	LimitedCircularFiles = 1,
	LimitedSequentialFiles = 3,
	SingleFileBoundedSize = 4,
	SingleFileUnboundedSize = 2,
	UnlimitedSequentialFiles = 0
}

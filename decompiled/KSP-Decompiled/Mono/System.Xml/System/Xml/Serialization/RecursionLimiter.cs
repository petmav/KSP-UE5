namespace System.Xml.Serialization;

internal class RecursionLimiter
{
	private int maxDepth;

	private int depth;

	private WorkItems deferredWorkItems;

	internal bool IsExceededLimit => depth > maxDepth;

	internal int Depth
	{
		get
		{
			return depth;
		}
		set
		{
			depth = value;
		}
	}

	internal WorkItems DeferredWorkItems
	{
		get
		{
			if (deferredWorkItems == null)
			{
				deferredWorkItems = new WorkItems();
			}
			return deferredWorkItems;
		}
	}

	internal RecursionLimiter()
	{
		depth = 0;
		maxDepth = (DiagnosticsSwitches.NonRecursiveTypeLoading.Enabled ? 1 : int.MaxValue);
	}
}

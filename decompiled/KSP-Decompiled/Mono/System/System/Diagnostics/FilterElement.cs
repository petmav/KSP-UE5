namespace System.Diagnostics;

internal class FilterElement : TypedElement
{
	public FilterElement()
		: base(typeof(TraceFilter))
	{
	}

	public TraceFilter GetRuntimeObject()
	{
		TraceFilter obj = (TraceFilter)BaseGetRuntimeObject();
		obj.initializeData = base.InitData;
		return obj;
	}

	internal TraceFilter RefreshRuntimeObject(TraceFilter filter)
	{
		if (Type.GetType(TypeName) != filter.GetType() || base.InitData != filter.initializeData)
		{
			_runtimeObject = null;
			return GetRuntimeObject();
		}
		return filter;
	}
}

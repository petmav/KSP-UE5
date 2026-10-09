namespace System.Diagnostics.Tracing;

[EventSource(Name = "Microsoft.Tasks.Nuget")]
internal class TplEtwProvider : EventSource
{
	public class Keywords
	{
		public const EventKeywords Debug = (EventKeywords)1L;
	}

	public static TplEtwProvider Log = new TplEtwProvider();

	public bool Debug => IsEnabled(EventLevel.Verbose, (EventKeywords)1L);

	public void DebugFacilityMessage(string Facility, string Message)
	{
		WriteEvent(1, Facility, Message);
	}

	public void DebugFacilityMessage1(string Facility, string Message, string Arg)
	{
		WriteEvent(2, Facility, Message, Arg);
	}

	public void SetActivityId(Guid Id)
	{
		WriteEvent(3, Id);
	}
}

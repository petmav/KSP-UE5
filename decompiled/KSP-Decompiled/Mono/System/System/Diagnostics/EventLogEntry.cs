using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Security.Permissions;
using Unity;

namespace System.Diagnostics;

[Serializable]
[DesignTimeVisible(false)]
[ToolboxItem(false)]
[PermissionSet(SecurityAction.LinkDemand, Unrestricted = true)]
public sealed class EventLogEntry : Component, ISerializable
{
	private string category;

	private short categoryNumber;

	private byte[] data;

	private EventLogEntryType entryType;

	private int eventID;

	private int index;

	private string machineName;

	private string message;

	private string[] replacementStrings;

	private string source;

	private DateTime timeGenerated;

	private DateTime timeWritten;

	private string userName;

	private long instanceId;

	[MonitoringDescription("The category of this event entry.")]
	public string Category => category;

	[MonitoringDescription("An ID for the category of this event entry.")]
	public short CategoryNumber => categoryNumber;

	[MonitoringDescription("Binary data associated with this event entry.")]
	public byte[] Data => data;

	[MonitoringDescription("The type of this event entry.")]
	public EventLogEntryType EntryType => entryType;

	[Obsolete("Use InstanceId")]
	[MonitoringDescription("An ID number for this event entry.")]
	public int EventID => eventID;

	[MonitoringDescription("Sequence numer of this event entry.")]
	public int Index => index;

	[MonitoringDescription("The instance ID for this event entry.")]
	[ComVisible(false)]
	public long InstanceId => instanceId;

	[MonitoringDescription("The Computer on which this event entry occured.")]
	public string MachineName => machineName;

	[Editor("System.ComponentModel.Design.BinaryEditor, System.Design, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a", "System.Drawing.Design.UITypeEditor, System.Drawing, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a")]
	[MonitoringDescription("The message of this event entry.")]
	public string Message => message;

	[MonitoringDescription("Application strings for this event entry.")]
	public string[] ReplacementStrings => replacementStrings;

	[MonitoringDescription("The source application of this event entry.")]
	public string Source => source;

	[MonitoringDescription("Generation time of this event entry.")]
	public DateTime TimeGenerated => timeGenerated;

	[MonitoringDescription("The time at which this event entry was written to the logfile.")]
	public DateTime TimeWritten => timeWritten;

	[MonitoringDescription("The name of a user associated with this event entry.")]
	public string UserName => userName;

	internal EventLogEntry(string category, short categoryNumber, int index, int eventID, string source, string message, string userName, string machineName, EventLogEntryType entryType, DateTime timeGenerated, DateTime timeWritten, byte[] data, string[] replacementStrings, long instanceId)
	{
		this.category = category;
		this.categoryNumber = categoryNumber;
		this.data = data;
		this.entryType = entryType;
		this.eventID = eventID;
		this.index = index;
		this.machineName = machineName;
		this.message = message;
		this.replacementStrings = replacementStrings;
		this.source = source;
		this.timeGenerated = timeGenerated;
		this.timeWritten = timeWritten;
		this.userName = userName;
		this.instanceId = instanceId;
	}

	[System.MonoTODO]
	private EventLogEntry(SerializationInfo info, StreamingContext context)
	{
	}

	public bool Equals(EventLogEntry otherEntry)
	{
		if (otherEntry == this)
		{
			return true;
		}
		if (otherEntry.Category == category && otherEntry.CategoryNumber == categoryNumber && otherEntry.Data.Equals(data) && otherEntry.EntryType == entryType && otherEntry.InstanceId == instanceId && otherEntry.Index == index && otherEntry.MachineName == machineName && otherEntry.Message == message && otherEntry.ReplacementStrings.Equals(replacementStrings) && otherEntry.Source == source && otherEntry.TimeGenerated.Equals(timeGenerated) && otherEntry.TimeWritten.Equals(timeWritten))
		{
			return otherEntry.UserName == userName;
		}
		return false;
	}

	[System.MonoTODO("Needs serialization support")]
	void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context)
	{
		throw new NotImplementedException();
	}

	internal EventLogEntry()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}

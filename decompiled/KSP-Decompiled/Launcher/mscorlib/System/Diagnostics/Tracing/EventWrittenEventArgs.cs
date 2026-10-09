using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Reflection;
using System.Security;
using Unity;

namespace System.Diagnostics.Tracing;

public class EventWrittenEventArgs : EventArgs
{
	private string m_message;

	private string m_eventName;

	private EventSource m_eventSource;

	private ReadOnlyCollection<string> m_payloadNames;

	internal EventTags m_tags;

	internal EventOpcode m_opcode;

	internal EventKeywords m_keywords;

	public string EventName
	{
		get
		{
			if (m_eventName != null || EventId < 0)
			{
				return m_eventName;
			}
			return m_eventSource.m_eventData[EventId].Name;
		}
		internal set
		{
			m_eventName = value;
		}
	}

	public int EventId { get; internal set; }

	public Guid ActivityId
	{
		[SecurityCritical]
		get
		{
			return EventSource.CurrentThreadActivityId;
		}
	}

	public Guid RelatedActivityId
	{
		[SecurityCritical]
		get;
		internal set; }

	public ReadOnlyCollection<object> Payload { get; internal set; }

	public ReadOnlyCollection<string> PayloadNames
	{
		get
		{
			if (m_payloadNames == null)
			{
				List<string> list = new List<string>();
				ParameterInfo[] parameters = m_eventSource.m_eventData[EventId].Parameters;
				foreach (ParameterInfo parameterInfo in parameters)
				{
					list.Add(parameterInfo.Name);
				}
				m_payloadNames = new ReadOnlyCollection<string>(list);
			}
			return m_payloadNames;
		}
		internal set
		{
			m_payloadNames = value;
		}
	}

	public EventSource EventSource => m_eventSource;

	public EventKeywords Keywords
	{
		get
		{
			if (EventId < 0)
			{
				return m_keywords;
			}
			return (EventKeywords)m_eventSource.m_eventData[EventId].Descriptor.Keywords;
		}
	}

	public EventOpcode Opcode
	{
		get
		{
			if (EventId < 0)
			{
				return m_opcode;
			}
			return (EventOpcode)m_eventSource.m_eventData[EventId].Descriptor.Opcode;
		}
	}

	public EventTask Task
	{
		get
		{
			if (EventId < 0)
			{
				return EventTask.None;
			}
			return (EventTask)m_eventSource.m_eventData[EventId].Descriptor.Task;
		}
	}

	public EventTags Tags
	{
		get
		{
			if (EventId < 0)
			{
				return m_tags;
			}
			return m_eventSource.m_eventData[EventId].Tags;
		}
	}

	public string Message
	{
		get
		{
			if (EventId < 0)
			{
				return m_message;
			}
			return m_eventSource.m_eventData[EventId].Message;
		}
		internal set
		{
			m_message = value;
		}
	}

	public EventChannel Channel
	{
		get
		{
			if (EventId < 0)
			{
				return EventChannel.None;
			}
			return (EventChannel)m_eventSource.m_eventData[EventId].Descriptor.Channel;
		}
	}

	public byte Version
	{
		get
		{
			if (EventId < 0)
			{
				return 0;
			}
			return m_eventSource.m_eventData[EventId].Descriptor.Version;
		}
	}

	public EventLevel Level
	{
		get
		{
			if (EventId < 0)
			{
				return EventLevel.LogAlways;
			}
			return (EventLevel)m_eventSource.m_eventData[EventId].Descriptor.Level;
		}
	}

	internal EventWrittenEventArgs(EventSource eventSource)
	{
		m_eventSource = eventSource;
	}

	internal EventWrittenEventArgs()
	{
		ThrowStub.ThrowNotSupportedException();
	}
}

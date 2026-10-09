using System.ComponentModel;

namespace System.Diagnostics;

[Designer("System.Diagnostics.Design.ProcessThreadDesigner, System.Design, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a")]
public class ProcessThread : Component
{
	[MonitoringDescription("The base priority of this thread.")]
	[System.MonoTODO]
	public int BasePriority => 0;

	[System.MonoTODO]
	[MonitoringDescription("The current priority of this thread.")]
	public int CurrentPriority => 0;

	[System.MonoTODO]
	[MonitoringDescription("The ID of this thread.")]
	public int Id => 0;

	[System.MonoTODO]
	[Browsable(false)]
	public int IdealProcessor
	{
		set
		{
		}
	}

	[System.MonoTODO]
	[MonitoringDescription("Thread gets a priority boot when interactively used by a user.")]
	public bool PriorityBoostEnabled
	{
		get
		{
			return false;
		}
		set
		{
		}
	}

	[MonitoringDescription("The priority level of this thread.")]
	[System.MonoTODO]
	public ThreadPriorityLevel PriorityLevel
	{
		get
		{
			return ThreadPriorityLevel.Idle;
		}
		set
		{
		}
	}

	[MonitoringDescription("The amount of CPU time used in privileged mode.")]
	[System.MonoTODO]
	public TimeSpan PrivilegedProcessorTime => new TimeSpan(0L);

	[Browsable(false)]
	[System.MonoTODO]
	public IntPtr ProcessorAffinity
	{
		set
		{
		}
	}

	[System.MonoTODO]
	[MonitoringDescription("The start address in memory of this thread.")]
	public IntPtr StartAddress => (IntPtr)0;

	[System.MonoTODO]
	[MonitoringDescription("The time this thread was started.")]
	public DateTime StartTime => new DateTime(0L);

	[System.MonoTODO]
	[MonitoringDescription("The current state of this thread.")]
	public ThreadState ThreadState => ThreadState.Initialized;

	[System.MonoTODO]
	[MonitoringDescription("The total amount of CPU time used.")]
	public TimeSpan TotalProcessorTime => new TimeSpan(0L);

	[MonitoringDescription("The amount of CPU time used in user mode.")]
	[System.MonoTODO]
	public TimeSpan UserProcessorTime => new TimeSpan(0L);

	[MonitoringDescription("The reason why this thread is waiting.")]
	[System.MonoTODO]
	public ThreadWaitReason WaitReason => ThreadWaitReason.Executive;

	[System.MonoTODO("Parse parameters")]
	internal ProcessThread()
	{
	}

	[System.MonoTODO]
	public void ResetIdealProcessor()
	{
	}
}

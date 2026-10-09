using System.Collections.Generic;

namespace System.Net;

internal static class DebugThreadTracking
{
	private class ThreadKindFrame : IDisposable
	{
		private readonly int _frameNumber;

		internal ThreadKindFrame()
		{
			_frameNumber = ThreadKindStack.Count;
		}

		void IDisposable.Dispose()
		{
			if (!Environment.HasShutdownStarted)
			{
				if (_frameNumber != ThreadKindStack.Count)
				{
					throw new System.Net.InternalException();
				}
				System.Net.ThreadKinds threadKinds = ThreadKindStack.Pop();
				if (CurrentThreadKind != threadKinds && System.Net.NetEventSource.IsEnabled && System.Net.NetEventSource.IsEnabled)
				{
					System.Net.NetEventSource.Info(this, $"Thread reverts:({CurrentThreadKind})", "Dispose");
				}
			}
		}
	}

	[ThreadStatic]
	private static Stack<System.Net.ThreadKinds> t_threadKindStack;

	private static Stack<System.Net.ThreadKinds> ThreadKindStack => t_threadKindStack ?? (t_threadKindStack = new Stack<System.Net.ThreadKinds>());

	internal static System.Net.ThreadKinds CurrentThreadKind
	{
		get
		{
			if (ThreadKindStack.Count <= 0)
			{
				return System.Net.ThreadKinds.Other;
			}
			return ThreadKindStack.Peek();
		}
	}

	internal static IDisposable SetThreadKind(System.Net.ThreadKinds kind)
	{
		if ((kind & System.Net.ThreadKinds.SourceMask) != 0)
		{
			throw new System.Net.InternalException();
		}
		if (Environment.HasShutdownStarted)
		{
			return null;
		}
		System.Net.ThreadKinds currentThreadKind = CurrentThreadKind;
		System.Net.ThreadKinds threadKinds = currentThreadKind & System.Net.ThreadKinds.SourceMask;
		if ((currentThreadKind & System.Net.ThreadKinds.User) != 0 && (kind & System.Net.ThreadKinds.System) != 0 && System.Net.NetEventSource.IsEnabled)
		{
			System.Net.NetEventSource.Error(null, "Thread changed from User to System; user's thread shouldn't be hijacked.", "SetThreadKind");
		}
		if ((currentThreadKind & System.Net.ThreadKinds.Async) != 0 && (kind & System.Net.ThreadKinds.Sync) != 0)
		{
			if (System.Net.NetEventSource.IsEnabled)
			{
				System.Net.NetEventSource.Error(null, "Thread changed from Async to Sync, may block an Async thread.", "SetThreadKind");
			}
		}
		else if ((currentThreadKind & (System.Net.ThreadKinds.CompletionPort | System.Net.ThreadKinds.Other)) == 0 && (kind & System.Net.ThreadKinds.Sync) != 0 && System.Net.NetEventSource.IsEnabled)
		{
			System.Net.NetEventSource.Error(null, "Thread from a limited resource changed to Sync, may deadlock or bottleneck.", "SetThreadKind");
		}
		ThreadKindStack.Push(((((kind & System.Net.ThreadKinds.OwnerMask) == 0) ? currentThreadKind : kind) & System.Net.ThreadKinds.OwnerMask) | ((((kind & System.Net.ThreadKinds.SyncMask) == 0) ? currentThreadKind : kind) & System.Net.ThreadKinds.SyncMask) | (kind & ~(System.Net.ThreadKinds.OwnerMask | System.Net.ThreadKinds.SyncMask)) | threadKinds);
		if (CurrentThreadKind != currentThreadKind && System.Net.NetEventSource.IsEnabled)
		{
			System.Net.NetEventSource.Info(null, $"Thread becomes:({CurrentThreadKind})", "SetThreadKind");
		}
		return new ThreadKindFrame();
	}

	internal static void SetThreadSource(System.Net.ThreadKinds source)
	{
		if ((source & System.Net.ThreadKinds.SourceMask) != source || source == System.Net.ThreadKinds.Unknown)
		{
			throw new ArgumentException("Must specify the thread source.", "source");
		}
		if (ThreadKindStack.Count == 0)
		{
			ThreadKindStack.Push(source);
			return;
		}
		if (ThreadKindStack.Count > 1)
		{
			if (System.Net.NetEventSource.IsEnabled)
			{
				System.Net.NetEventSource.Error(null, "SetThreadSource must be called at the base of the stack, or the stack has been corrupted.", "SetThreadSource");
			}
			while (ThreadKindStack.Count > 1)
			{
				ThreadKindStack.Pop();
			}
		}
		if (ThreadKindStack.Peek() != source)
		{
			if (System.Net.NetEventSource.IsEnabled)
			{
				System.Net.NetEventSource.Error(null, "The stack has been corrupted.", "SetThreadSource");
			}
			System.Net.ThreadKinds threadKinds = ThreadKindStack.Pop() & System.Net.ThreadKinds.SourceMask;
			if (threadKinds != source && threadKinds != System.Net.ThreadKinds.Other && System.Net.NetEventSource.IsEnabled)
			{
				System.Net.NetEventSource.Fail(null, $"Thread source changed.|Was:({threadKinds}) Now:({source})", "SetThreadSource");
			}
			ThreadKindStack.Push(source);
		}
	}
}

using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Runtime.ExceptionServices;
using System.Security;

namespace System.Threading.Tasks;

internal class TaskExceptionHolder
{
	private static readonly bool s_failFastOnUnobservedException = ShouldFailFastOnUnobservedException();

	private static volatile bool s_domainUnloadStarted;

	private static volatile EventHandler s_adUnloadEventHandler;

	private readonly Task m_task;

	private volatile List<ExceptionDispatchInfo> m_faultExceptions;

	private ExceptionDispatchInfo m_cancellationException;

	private volatile bool m_isHandled;

	internal bool ContainsFaultList => m_faultExceptions != null;

	internal TaskExceptionHolder(Task task)
	{
		m_task = task;
		EnsureADUnloadCallbackRegistered();
	}

	[SecuritySafeCritical]
	private static bool ShouldFailFastOnUnobservedException()
	{
		return CLRConfig.CheckThrowUnobservedTaskExceptions();
	}

	private static void EnsureADUnloadCallbackRegistered()
	{
		if (s_adUnloadEventHandler == null && Interlocked.CompareExchange(ref s_adUnloadEventHandler, AppDomainUnloadCallback, null) == null)
		{
			AppDomain.CurrentDomain.DomainUnload += s_adUnloadEventHandler;
		}
	}

	private static void AppDomainUnloadCallback(object sender, EventArgs e)
	{
		s_domainUnloadStarted = true;
	}

	~TaskExceptionHolder()
	{
		if (m_faultExceptions == null || (m_isHandled || (Environment.HasShutdownStarted || (AppDomain.CurrentDomain.IsFinalizingForUnload() || s_domainUnloadStarted))))
		{
			return;
		}
		foreach (ExceptionDispatchInfo faultException in m_faultExceptions)
		{
			Exception sourceException = faultException.SourceException;
			if (sourceException is AggregateException ex)
			{
				foreach (Exception innerException in ex.Flatten().InnerExceptions)
				{
					if (innerException is ThreadAbortException)
					{
						return;
					}
				}
			}
			else
			{
				if (sourceException is ThreadAbortException)
				{
					return;
				}
			}
		}
		AggregateException ex2 = new AggregateException(Environment.GetResourceString("A Task's exception(s) were not observed either by Waiting on the Task or accessing its Exception property. As a result, the unobserved exception was rethrown by the finalizer thread."), m_faultExceptions);
		UnobservedTaskExceptionEventArgs unobservedTaskExceptionEventArgs = new UnobservedTaskExceptionEventArgs(ex2);
		TaskScheduler.PublishUnobservedTaskException(m_task, unobservedTaskExceptionEventArgs);
		if (s_failFastOnUnobservedException && !unobservedTaskExceptionEventArgs.m_observed)
		{
			throw ex2;
		}
	}

	internal void Add(object exceptionObject)
	{
		Add(exceptionObject, representsCancellation: false);
	}

	internal void Add(object exceptionObject, bool representsCancellation)
	{
		if (representsCancellation)
		{
			SetCancellationException(exceptionObject);
		}
		else
		{
			AddFaultException(exceptionObject);
		}
	}

	private void SetCancellationException(object exceptionObject)
	{
		if (exceptionObject is OperationCanceledException source)
		{
			m_cancellationException = ExceptionDispatchInfo.Capture(source);
		}
		else
		{
			ExceptionDispatchInfo cancellationException = exceptionObject as ExceptionDispatchInfo;
			m_cancellationException = cancellationException;
		}
		MarkAsHandled(calledFromFinalizer: false);
	}

	private void AddFaultException(object exceptionObject)
	{
		List<ExceptionDispatchInfo> list = m_faultExceptions;
		if (list == null)
		{
			list = (m_faultExceptions = new List<ExceptionDispatchInfo>(1));
		}
		if (exceptionObject is Exception source)
		{
			list.Add(ExceptionDispatchInfo.Capture(source));
		}
		else if (exceptionObject is ExceptionDispatchInfo item)
		{
			list.Add(item);
		}
		else if (exceptionObject is IEnumerable<Exception> enumerable)
		{
			foreach (Exception item2 in enumerable)
			{
				list.Add(ExceptionDispatchInfo.Capture(item2));
			}
		}
		else
		{
			if (!(exceptionObject is IEnumerable<ExceptionDispatchInfo> collection))
			{
				throw new ArgumentException(Environment.GetResourceString("(Internal)Expected an Exception or an IEnumerable<Exception>"), "exceptionObject");
			}
			list.AddRange(collection);
		}
		for (int i = 0; i < list.Count; i++)
		{
			Type type = list[i].SourceException.GetType();
			if (type != typeof(ThreadAbortException) && type != typeof(AppDomainUnloadedException))
			{
				MarkAsUnhandled();
				break;
			}
			if (i == list.Count - 1)
			{
				MarkAsHandled(calledFromFinalizer: false);
			}
		}
	}

	private void MarkAsUnhandled()
	{
		if (m_isHandled)
		{
			GC.ReRegisterForFinalize(this);
			m_isHandled = false;
		}
	}

	internal void MarkAsHandled(bool calledFromFinalizer)
	{
		if (!m_isHandled)
		{
			if (!calledFromFinalizer)
			{
				GC.SuppressFinalize(this);
			}
			m_isHandled = true;
		}
	}

	internal AggregateException CreateExceptionObject(bool calledFromFinalizer, Exception includeThisException)
	{
		List<ExceptionDispatchInfo> faultExceptions = m_faultExceptions;
		MarkAsHandled(calledFromFinalizer);
		if (includeThisException == null)
		{
			return new AggregateException(faultExceptions);
		}
		Exception[] array = new Exception[faultExceptions.Count + 1];
		for (int i = 0; i < array.Length - 1; i++)
		{
			array[i] = faultExceptions[i].SourceException;
		}
		array[array.Length - 1] = includeThisException;
		return new AggregateException(array);
	}

	internal ReadOnlyCollection<ExceptionDispatchInfo> GetExceptionDispatchInfos()
	{
		List<ExceptionDispatchInfo> faultExceptions = m_faultExceptions;
		MarkAsHandled(calledFromFinalizer: false);
		return new ReadOnlyCollection<ExceptionDispatchInfo>(faultExceptions);
	}

	internal ExceptionDispatchInfo GetCancellationExceptionDispatchInfo()
	{
		return m_cancellationException;
	}
}

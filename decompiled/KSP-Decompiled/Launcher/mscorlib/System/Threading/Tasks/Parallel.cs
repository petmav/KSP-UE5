using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Security.Permissions;

namespace System.Threading.Tasks;

[HostProtection(SecurityAction.LinkDemand, Synchronization = true, ExternalThreading = true)]
public static class Parallel
{
	internal struct LoopTimer
	{
		private const int s_BaseNotifyPeriodMS = 100;

		private const int s_NotifyPeriodIncrementMS = 50;

		private int m_timeLimit;

		public LoopTimer(int nWorkerTaskIndex)
		{
			int num = 100 + nWorkerTaskIndex % PlatformHelper.ProcessorCount * 50;
			m_timeLimit = Environment.TickCount + num;
		}

		public bool LimitExceeded()
		{
			return Environment.TickCount > m_timeLimit;
		}
	}

	internal static int s_forkJoinContextID;

	internal const int DEFAULT_LOOP_STRIDE = 16;

	internal static ParallelOptions s_defaultParallelOptions = new ParallelOptions();

	public static void Invoke(params Action[] actions)
	{
		Invoke(s_defaultParallelOptions, actions);
	}

	public static void Invoke(ParallelOptions parallelOptions, params Action[] actions)
	{
		if (actions == null)
		{
			throw new ArgumentNullException("actions");
		}
		if (parallelOptions == null)
		{
			throw new ArgumentNullException("parallelOptions");
		}
		if (parallelOptions.CancellationToken.CanBeCanceled && AppContextSwitches.ThrowExceptionIfDisposedCancellationTokenSource)
		{
			parallelOptions.CancellationToken.ThrowIfSourceDisposed();
		}
		if (parallelOptions.CancellationToken.IsCancellationRequested)
		{
			throw new OperationCanceledException(parallelOptions.CancellationToken);
		}
		Action[] actionsCopy = new Action[actions.Length];
		for (int i = 0; i < actionsCopy.Length; i++)
		{
			actionsCopy[i] = actions[i];
			if (actionsCopy[i] == null)
			{
				throw new ArgumentException(Environment.GetResourceString("One of the actions was null."));
			}
		}
		if (actionsCopy.Length < 1)
		{
			return;
		}
		if (actionsCopy.Length > 10 || (parallelOptions.MaxDegreeOfParallelism != -1 && parallelOptions.MaxDegreeOfParallelism < actionsCopy.Length))
		{
			ConcurrentQueue<Exception> exceptionQ = null;
			try
			{
				int actionIndex = 0;
				ParallelForReplicatingTask parallelForReplicatingTask = new ParallelForReplicatingTask(parallelOptions, delegate
				{
					for (int num = Interlocked.Increment(ref actionIndex); num <= actionsCopy.Length; num = Interlocked.Increment(ref actionIndex))
					{
						try
						{
							actionsCopy[num - 1]();
						}
						catch (Exception item)
						{
							LazyInitializer.EnsureInitialized(ref exceptionQ, () => new ConcurrentQueue<Exception>());
							exceptionQ.Enqueue(item);
						}
						if (parallelOptions.CancellationToken.IsCancellationRequested)
						{
							throw new OperationCanceledException(parallelOptions.CancellationToken);
						}
					}
				}, TaskCreationOptions.None, InternalTaskOptions.SelfReplicating);
				parallelForReplicatingTask.RunSynchronously(parallelOptions.EffectiveTaskScheduler);
				parallelForReplicatingTask.Wait();
			}
			catch (Exception ex)
			{
				LazyInitializer.EnsureInitialized(ref exceptionQ, () => new ConcurrentQueue<Exception>());
				if (ex is AggregateException ex2)
				{
					foreach (Exception innerException in ex2.InnerExceptions)
					{
						exceptionQ.Enqueue(innerException);
					}
				}
				else
				{
					exceptionQ.Enqueue(ex);
				}
			}
			if (exceptionQ == null || exceptionQ.Count <= 0)
			{
				return;
			}
			ThrowIfReducableToSingleOCE(exceptionQ, parallelOptions.CancellationToken);
			throw new AggregateException(exceptionQ);
		}
		Task[] array = new Task[actionsCopy.Length];
		if (parallelOptions.CancellationToken.IsCancellationRequested)
		{
			throw new OperationCanceledException(parallelOptions.CancellationToken);
		}
		for (int j = 1; j < array.Length; j++)
		{
			array[j] = Task.Factory.StartNew(actionsCopy[j], parallelOptions.CancellationToken, TaskCreationOptions.None, InternalTaskOptions.None, parallelOptions.EffectiveTaskScheduler);
		}
		array[0] = new Task(actionsCopy[0]);
		array[0].RunSynchronously(parallelOptions.EffectiveTaskScheduler);
		try
		{
			if (array.Length <= 4)
			{
				Task.FastWaitAll(array);
			}
			else
			{
				Task.WaitAll(array);
			}
		}
		catch (AggregateException ex3)
		{
			ThrowIfReducableToSingleOCE(ex3.InnerExceptions, parallelOptions.CancellationToken);
			throw;
		}
		finally
		{
			for (int k = 0; k < array.Length; k++)
			{
				if (array[k].IsCompleted)
				{
					array[k].Dispose();
				}
			}
		}
	}

	public static ParallelLoopResult For(int fromInclusive, int toExclusive, Action<int> body)
	{
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		return ForWorker<object>(fromInclusive, toExclusive, s_defaultParallelOptions, body, null, null, null, null);
	}

	public static ParallelLoopResult For(long fromInclusive, long toExclusive, Action<long> body)
	{
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		return ForWorker64<object>(fromInclusive, toExclusive, s_defaultParallelOptions, body, null, null, null, null);
	}

	public static ParallelLoopResult For(int fromInclusive, int toExclusive, ParallelOptions parallelOptions, Action<int> body)
	{
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		if (parallelOptions == null)
		{
			throw new ArgumentNullException("parallelOptions");
		}
		return ForWorker<object>(fromInclusive, toExclusive, parallelOptions, body, null, null, null, null);
	}

	public static ParallelLoopResult For(long fromInclusive, long toExclusive, ParallelOptions parallelOptions, Action<long> body)
	{
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		if (parallelOptions == null)
		{
			throw new ArgumentNullException("parallelOptions");
		}
		return ForWorker64<object>(fromInclusive, toExclusive, parallelOptions, body, null, null, null, null);
	}

	public static ParallelLoopResult For(int fromInclusive, int toExclusive, Action<int, ParallelLoopState> body)
	{
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		return ForWorker<object>(fromInclusive, toExclusive, s_defaultParallelOptions, null, body, null, null, null);
	}

	public static ParallelLoopResult For(long fromInclusive, long toExclusive, Action<long, ParallelLoopState> body)
	{
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		return ForWorker64<object>(fromInclusive, toExclusive, s_defaultParallelOptions, null, body, null, null, null);
	}

	public static ParallelLoopResult For(int fromInclusive, int toExclusive, ParallelOptions parallelOptions, Action<int, ParallelLoopState> body)
	{
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		if (parallelOptions == null)
		{
			throw new ArgumentNullException("parallelOptions");
		}
		return ForWorker<object>(fromInclusive, toExclusive, parallelOptions, null, body, null, null, null);
	}

	public static ParallelLoopResult For(long fromInclusive, long toExclusive, ParallelOptions parallelOptions, Action<long, ParallelLoopState> body)
	{
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		if (parallelOptions == null)
		{
			throw new ArgumentNullException("parallelOptions");
		}
		return ForWorker64<object>(fromInclusive, toExclusive, parallelOptions, null, body, null, null, null);
	}

	public static ParallelLoopResult For<TLocal>(int fromInclusive, int toExclusive, Func<TLocal> localInit, Func<int, ParallelLoopState, TLocal, TLocal> body, Action<TLocal> localFinally)
	{
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		if (localInit == null)
		{
			throw new ArgumentNullException("localInit");
		}
		if (localFinally == null)
		{
			throw new ArgumentNullException("localFinally");
		}
		return ForWorker(fromInclusive, toExclusive, s_defaultParallelOptions, null, null, body, localInit, localFinally);
	}

	public static ParallelLoopResult For<TLocal>(long fromInclusive, long toExclusive, Func<TLocal> localInit, Func<long, ParallelLoopState, TLocal, TLocal> body, Action<TLocal> localFinally)
	{
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		if (localInit == null)
		{
			throw new ArgumentNullException("localInit");
		}
		if (localFinally == null)
		{
			throw new ArgumentNullException("localFinally");
		}
		return ForWorker64(fromInclusive, toExclusive, s_defaultParallelOptions, null, null, body, localInit, localFinally);
	}

	public static ParallelLoopResult For<TLocal>(int fromInclusive, int toExclusive, ParallelOptions parallelOptions, Func<TLocal> localInit, Func<int, ParallelLoopState, TLocal, TLocal> body, Action<TLocal> localFinally)
	{
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		if (localInit == null)
		{
			throw new ArgumentNullException("localInit");
		}
		if (localFinally == null)
		{
			throw new ArgumentNullException("localFinally");
		}
		if (parallelOptions == null)
		{
			throw new ArgumentNullException("parallelOptions");
		}
		return ForWorker(fromInclusive, toExclusive, parallelOptions, null, null, body, localInit, localFinally);
	}

	public static ParallelLoopResult For<TLocal>(long fromInclusive, long toExclusive, ParallelOptions parallelOptions, Func<TLocal> localInit, Func<long, ParallelLoopState, TLocal, TLocal> body, Action<TLocal> localFinally)
	{
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		if (localInit == null)
		{
			throw new ArgumentNullException("localInit");
		}
		if (localFinally == null)
		{
			throw new ArgumentNullException("localFinally");
		}
		if (parallelOptions == null)
		{
			throw new ArgumentNullException("parallelOptions");
		}
		return ForWorker64(fromInclusive, toExclusive, parallelOptions, null, null, body, localInit, localFinally);
	}

	private static ParallelLoopResult ForWorker<TLocal>(int fromInclusive, int toExclusive, ParallelOptions parallelOptions, Action<int> body, Action<int, ParallelLoopState> bodyWithState, Func<int, ParallelLoopState, TLocal, TLocal> bodyWithLocal, Func<TLocal> localInit, Action<TLocal> localFinally)
	{
		ParallelLoopResult result = default(ParallelLoopResult);
		if (toExclusive <= fromInclusive)
		{
			result.m_completed = true;
			return result;
		}
		ParallelLoopStateFlags32 sharedPStateFlags = new ParallelLoopStateFlags32();
		TaskCreationOptions creationOptions = TaskCreationOptions.None;
		InternalTaskOptions internalOptions = InternalTaskOptions.SelfReplicating;
		if (parallelOptions.CancellationToken.IsCancellationRequested)
		{
			throw new OperationCanceledException(parallelOptions.CancellationToken);
		}
		int nNumExpectedWorkers = ((parallelOptions.EffectiveMaxConcurrencyLevel == -1) ? PlatformHelper.ProcessorCount : parallelOptions.EffectiveMaxConcurrencyLevel);
		RangeManager rangeManager = new RangeManager(fromInclusive, toExclusive, 1L, nNumExpectedWorkers);
		OperationCanceledException oce = null;
		CancellationTokenRegistration cancellationTokenRegistration = default(CancellationTokenRegistration);
		if (parallelOptions.CancellationToken.CanBeCanceled)
		{
			cancellationTokenRegistration = parallelOptions.CancellationToken.InternalRegisterWithoutEC(delegate
			{
				sharedPStateFlags.Cancel();
				oce = new OperationCanceledException(parallelOptions.CancellationToken);
			}, null);
		}
		ParallelForReplicatingTask rootTask = null;
		try
		{
			rootTask = new ParallelForReplicatingTask(parallelOptions, delegate
			{
				Task internalCurrent = Task.InternalCurrent;
				bool flag = internalCurrent == rootTask;
				RangeWorker rangeWorker = default(RangeWorker);
				object savedStateFromPreviousReplica = internalCurrent.SavedStateFromPreviousReplica;
				rangeWorker = ((!(savedStateFromPreviousReplica is RangeWorker)) ? rangeManager.RegisterNewWorker() : ((RangeWorker)savedStateFromPreviousReplica));
				if (!rangeWorker.FindNewWork32(out var nFromInclusiveLocal, out var nToExclusiveLocal) || sharedPStateFlags.ShouldExitLoop(nFromInclusiveLocal))
				{
					return;
				}
				TLocal val = default(TLocal);
				bool flag2 = false;
				try
				{
					ParallelLoopState32 parallelLoopState = null;
					if (bodyWithState != null)
					{
						parallelLoopState = new ParallelLoopState32(sharedPStateFlags);
					}
					else if (bodyWithLocal != null)
					{
						parallelLoopState = new ParallelLoopState32(sharedPStateFlags);
						if (localInit != null)
						{
							val = localInit();
							flag2 = true;
						}
					}
					LoopTimer loopTimer = new LoopTimer(rootTask.ActiveChildCount);
					do
					{
						if (body != null)
						{
							for (int i = nFromInclusiveLocal; i < nToExclusiveLocal; i++)
							{
								if (sharedPStateFlags.LoopStateFlags != ParallelLoopStateFlags.PLS_NONE && sharedPStateFlags.ShouldExitLoop())
								{
									break;
								}
								body(i);
							}
						}
						else if (bodyWithState != null)
						{
							for (int j = nFromInclusiveLocal; j < nToExclusiveLocal && (sharedPStateFlags.LoopStateFlags == ParallelLoopStateFlags.PLS_NONE || !sharedPStateFlags.ShouldExitLoop(j)); j++)
							{
								parallelLoopState.CurrentIteration = j;
								bodyWithState(j, parallelLoopState);
							}
						}
						else
						{
							for (int k = nFromInclusiveLocal; k < nToExclusiveLocal && (sharedPStateFlags.LoopStateFlags == ParallelLoopStateFlags.PLS_NONE || !sharedPStateFlags.ShouldExitLoop(k)); k++)
							{
								parallelLoopState.CurrentIteration = k;
								val = bodyWithLocal(k, parallelLoopState, val);
							}
						}
						if (!flag && loopTimer.LimitExceeded())
						{
							internalCurrent.SavedStateForNextReplica = rangeWorker;
							break;
						}
					}
					while (rangeWorker.FindNewWork32(out nFromInclusiveLocal, out nToExclusiveLocal) && (sharedPStateFlags.LoopStateFlags == ParallelLoopStateFlags.PLS_NONE || !sharedPStateFlags.ShouldExitLoop(nFromInclusiveLocal)));
				}
				catch
				{
					sharedPStateFlags.SetExceptional();
					throw;
				}
				finally
				{
					if (localFinally != null && flag2)
					{
						localFinally(val);
					}
				}
			}, creationOptions, internalOptions);
			rootTask.RunSynchronously(parallelOptions.EffectiveTaskScheduler);
			rootTask.Wait();
			if (parallelOptions.CancellationToken.CanBeCanceled)
			{
				cancellationTokenRegistration.Dispose();
			}
			if (oce != null)
			{
				throw oce;
			}
		}
		catch (AggregateException ex)
		{
			if (parallelOptions.CancellationToken.CanBeCanceled)
			{
				cancellationTokenRegistration.Dispose();
			}
			ThrowIfReducableToSingleOCE(ex.InnerExceptions, parallelOptions.CancellationToken);
			throw;
		}
		catch (TaskSchedulerException)
		{
			if (parallelOptions.CancellationToken.CanBeCanceled)
			{
				cancellationTokenRegistration.Dispose();
			}
			throw;
		}
		finally
		{
			int loopStateFlags = sharedPStateFlags.LoopStateFlags;
			result.m_completed = loopStateFlags == ParallelLoopStateFlags.PLS_NONE;
			if ((loopStateFlags & ParallelLoopStateFlags.PLS_BROKEN) != 0)
			{
				result.m_lowestBreakIteration = sharedPStateFlags.LowestBreakIteration;
			}
			if (rootTask != null && rootTask.IsCompleted)
			{
				rootTask.Dispose();
			}
		}
		return result;
	}

	private static ParallelLoopResult ForWorker64<TLocal>(long fromInclusive, long toExclusive, ParallelOptions parallelOptions, Action<long> body, Action<long, ParallelLoopState> bodyWithState, Func<long, ParallelLoopState, TLocal, TLocal> bodyWithLocal, Func<TLocal> localInit, Action<TLocal> localFinally)
	{
		ParallelLoopResult result = default(ParallelLoopResult);
		if (toExclusive <= fromInclusive)
		{
			result.m_completed = true;
			return result;
		}
		ParallelLoopStateFlags64 sharedPStateFlags = new ParallelLoopStateFlags64();
		TaskCreationOptions creationOptions = TaskCreationOptions.None;
		InternalTaskOptions internalOptions = InternalTaskOptions.SelfReplicating;
		if (parallelOptions.CancellationToken.IsCancellationRequested)
		{
			throw new OperationCanceledException(parallelOptions.CancellationToken);
		}
		int nNumExpectedWorkers = ((parallelOptions.EffectiveMaxConcurrencyLevel == -1) ? PlatformHelper.ProcessorCount : parallelOptions.EffectiveMaxConcurrencyLevel);
		RangeManager rangeManager = new RangeManager(fromInclusive, toExclusive, 1L, nNumExpectedWorkers);
		OperationCanceledException oce = null;
		CancellationTokenRegistration cancellationTokenRegistration = default(CancellationTokenRegistration);
		if (parallelOptions.CancellationToken.CanBeCanceled)
		{
			cancellationTokenRegistration = parallelOptions.CancellationToken.InternalRegisterWithoutEC(delegate
			{
				sharedPStateFlags.Cancel();
				oce = new OperationCanceledException(parallelOptions.CancellationToken);
			}, null);
		}
		ParallelForReplicatingTask rootTask = null;
		try
		{
			rootTask = new ParallelForReplicatingTask(parallelOptions, delegate
			{
				Task internalCurrent = Task.InternalCurrent;
				bool flag = internalCurrent == rootTask;
				RangeWorker rangeWorker = default(RangeWorker);
				object savedStateFromPreviousReplica = internalCurrent.SavedStateFromPreviousReplica;
				rangeWorker = ((!(savedStateFromPreviousReplica is RangeWorker)) ? rangeManager.RegisterNewWorker() : ((RangeWorker)savedStateFromPreviousReplica));
				if (!rangeWorker.FindNewWork(out var nFromInclusiveLocal, out var nToExclusiveLocal) || sharedPStateFlags.ShouldExitLoop(nFromInclusiveLocal))
				{
					return;
				}
				TLocal val = default(TLocal);
				bool flag2 = false;
				try
				{
					ParallelLoopState64 parallelLoopState = null;
					if (bodyWithState != null)
					{
						parallelLoopState = new ParallelLoopState64(sharedPStateFlags);
					}
					else if (bodyWithLocal != null)
					{
						parallelLoopState = new ParallelLoopState64(sharedPStateFlags);
						if (localInit != null)
						{
							val = localInit();
							flag2 = true;
						}
					}
					LoopTimer loopTimer = new LoopTimer(rootTask.ActiveChildCount);
					do
					{
						if (body != null)
						{
							for (long num = nFromInclusiveLocal; num < nToExclusiveLocal; num++)
							{
								if (sharedPStateFlags.LoopStateFlags != ParallelLoopStateFlags.PLS_NONE && sharedPStateFlags.ShouldExitLoop())
								{
									break;
								}
								body(num);
							}
						}
						else if (bodyWithState != null)
						{
							for (long num2 = nFromInclusiveLocal; num2 < nToExclusiveLocal && (sharedPStateFlags.LoopStateFlags == ParallelLoopStateFlags.PLS_NONE || !sharedPStateFlags.ShouldExitLoop(num2)); num2++)
							{
								parallelLoopState.CurrentIteration = num2;
								bodyWithState(num2, parallelLoopState);
							}
						}
						else
						{
							for (long num3 = nFromInclusiveLocal; num3 < nToExclusiveLocal && (sharedPStateFlags.LoopStateFlags == ParallelLoopStateFlags.PLS_NONE || !sharedPStateFlags.ShouldExitLoop(num3)); num3++)
							{
								parallelLoopState.CurrentIteration = num3;
								val = bodyWithLocal(num3, parallelLoopState, val);
							}
						}
						if (!flag && loopTimer.LimitExceeded())
						{
							internalCurrent.SavedStateForNextReplica = rangeWorker;
							break;
						}
					}
					while (rangeWorker.FindNewWork(out nFromInclusiveLocal, out nToExclusiveLocal) && (sharedPStateFlags.LoopStateFlags == ParallelLoopStateFlags.PLS_NONE || !sharedPStateFlags.ShouldExitLoop(nFromInclusiveLocal)));
				}
				catch
				{
					sharedPStateFlags.SetExceptional();
					throw;
				}
				finally
				{
					if (localFinally != null && flag2)
					{
						localFinally(val);
					}
				}
			}, creationOptions, internalOptions);
			rootTask.RunSynchronously(parallelOptions.EffectiveTaskScheduler);
			rootTask.Wait();
			if (parallelOptions.CancellationToken.CanBeCanceled)
			{
				cancellationTokenRegistration.Dispose();
			}
			if (oce != null)
			{
				throw oce;
			}
		}
		catch (AggregateException ex)
		{
			if (parallelOptions.CancellationToken.CanBeCanceled)
			{
				cancellationTokenRegistration.Dispose();
			}
			ThrowIfReducableToSingleOCE(ex.InnerExceptions, parallelOptions.CancellationToken);
			throw;
		}
		catch (TaskSchedulerException)
		{
			if (parallelOptions.CancellationToken.CanBeCanceled)
			{
				cancellationTokenRegistration.Dispose();
			}
			throw;
		}
		finally
		{
			int loopStateFlags = sharedPStateFlags.LoopStateFlags;
			result.m_completed = loopStateFlags == ParallelLoopStateFlags.PLS_NONE;
			if ((loopStateFlags & ParallelLoopStateFlags.PLS_BROKEN) != 0)
			{
				result.m_lowestBreakIteration = sharedPStateFlags.LowestBreakIteration;
			}
			if (rootTask != null && rootTask.IsCompleted)
			{
				rootTask.Dispose();
			}
		}
		return result;
	}

	public static ParallelLoopResult ForEach<TSource>(IEnumerable<TSource> source, Action<TSource> body)
	{
		if (source == null)
		{
			throw new ArgumentNullException("source");
		}
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		return ForEachWorker<TSource, object>(source, s_defaultParallelOptions, body, null, null, null, null, null, null);
	}

	public static ParallelLoopResult ForEach<TSource>(IEnumerable<TSource> source, ParallelOptions parallelOptions, Action<TSource> body)
	{
		if (source == null)
		{
			throw new ArgumentNullException("source");
		}
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		if (parallelOptions == null)
		{
			throw new ArgumentNullException("parallelOptions");
		}
		return ForEachWorker<TSource, object>(source, parallelOptions, body, null, null, null, null, null, null);
	}

	public static ParallelLoopResult ForEach<TSource>(IEnumerable<TSource> source, Action<TSource, ParallelLoopState> body)
	{
		if (source == null)
		{
			throw new ArgumentNullException("source");
		}
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		return ForEachWorker<TSource, object>(source, s_defaultParallelOptions, null, body, null, null, null, null, null);
	}

	public static ParallelLoopResult ForEach<TSource>(IEnumerable<TSource> source, ParallelOptions parallelOptions, Action<TSource, ParallelLoopState> body)
	{
		if (source == null)
		{
			throw new ArgumentNullException("source");
		}
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		if (parallelOptions == null)
		{
			throw new ArgumentNullException("parallelOptions");
		}
		return ForEachWorker<TSource, object>(source, parallelOptions, null, body, null, null, null, null, null);
	}

	public static ParallelLoopResult ForEach<TSource>(IEnumerable<TSource> source, Action<TSource, ParallelLoopState, long> body)
	{
		if (source == null)
		{
			throw new ArgumentNullException("source");
		}
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		return ForEachWorker<TSource, object>(source, s_defaultParallelOptions, null, null, body, null, null, null, null);
	}

	public static ParallelLoopResult ForEach<TSource>(IEnumerable<TSource> source, ParallelOptions parallelOptions, Action<TSource, ParallelLoopState, long> body)
	{
		if (source == null)
		{
			throw new ArgumentNullException("source");
		}
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		if (parallelOptions == null)
		{
			throw new ArgumentNullException("parallelOptions");
		}
		return ForEachWorker<TSource, object>(source, parallelOptions, null, null, body, null, null, null, null);
	}

	public static ParallelLoopResult ForEach<TSource, TLocal>(IEnumerable<TSource> source, Func<TLocal> localInit, Func<TSource, ParallelLoopState, TLocal, TLocal> body, Action<TLocal> localFinally)
	{
		if (source == null)
		{
			throw new ArgumentNullException("source");
		}
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		if (localInit == null)
		{
			throw new ArgumentNullException("localInit");
		}
		if (localFinally == null)
		{
			throw new ArgumentNullException("localFinally");
		}
		return ForEachWorker(source, s_defaultParallelOptions, null, null, null, body, null, localInit, localFinally);
	}

	public static ParallelLoopResult ForEach<TSource, TLocal>(IEnumerable<TSource> source, ParallelOptions parallelOptions, Func<TLocal> localInit, Func<TSource, ParallelLoopState, TLocal, TLocal> body, Action<TLocal> localFinally)
	{
		if (source == null)
		{
			throw new ArgumentNullException("source");
		}
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		if (localInit == null)
		{
			throw new ArgumentNullException("localInit");
		}
		if (localFinally == null)
		{
			throw new ArgumentNullException("localFinally");
		}
		if (parallelOptions == null)
		{
			throw new ArgumentNullException("parallelOptions");
		}
		return ForEachWorker(source, parallelOptions, null, null, null, body, null, localInit, localFinally);
	}

	public static ParallelLoopResult ForEach<TSource, TLocal>(IEnumerable<TSource> source, Func<TLocal> localInit, Func<TSource, ParallelLoopState, long, TLocal, TLocal> body, Action<TLocal> localFinally)
	{
		if (source == null)
		{
			throw new ArgumentNullException("source");
		}
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		if (localInit == null)
		{
			throw new ArgumentNullException("localInit");
		}
		if (localFinally == null)
		{
			throw new ArgumentNullException("localFinally");
		}
		return ForEachWorker(source, s_defaultParallelOptions, null, null, null, null, body, localInit, localFinally);
	}

	public static ParallelLoopResult ForEach<TSource, TLocal>(IEnumerable<TSource> source, ParallelOptions parallelOptions, Func<TLocal> localInit, Func<TSource, ParallelLoopState, long, TLocal, TLocal> body, Action<TLocal> localFinally)
	{
		if (source == null)
		{
			throw new ArgumentNullException("source");
		}
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		if (localInit == null)
		{
			throw new ArgumentNullException("localInit");
		}
		if (localFinally == null)
		{
			throw new ArgumentNullException("localFinally");
		}
		if (parallelOptions == null)
		{
			throw new ArgumentNullException("parallelOptions");
		}
		return ForEachWorker(source, parallelOptions, null, null, null, null, body, localInit, localFinally);
	}

	private static ParallelLoopResult ForEachWorker<TSource, TLocal>(IEnumerable<TSource> source, ParallelOptions parallelOptions, Action<TSource> body, Action<TSource, ParallelLoopState> bodyWithState, Action<TSource, ParallelLoopState, long> bodyWithStateAndIndex, Func<TSource, ParallelLoopState, TLocal, TLocal> bodyWithStateAndLocal, Func<TSource, ParallelLoopState, long, TLocal, TLocal> bodyWithEverything, Func<TLocal> localInit, Action<TLocal> localFinally)
	{
		if (parallelOptions.CancellationToken.IsCancellationRequested)
		{
			throw new OperationCanceledException(parallelOptions.CancellationToken);
		}
		if (source is TSource[] array)
		{
			return ForEachWorker(array, parallelOptions, body, bodyWithState, bodyWithStateAndIndex, bodyWithStateAndLocal, bodyWithEverything, localInit, localFinally);
		}
		if (source is IList<TSource> list)
		{
			return ForEachWorker(list, parallelOptions, body, bodyWithState, bodyWithStateAndIndex, bodyWithStateAndLocal, bodyWithEverything, localInit, localFinally);
		}
		return PartitionerForEachWorker(Partitioner.Create(source), parallelOptions, body, bodyWithState, bodyWithStateAndIndex, bodyWithStateAndLocal, bodyWithEverything, localInit, localFinally);
	}

	private static ParallelLoopResult ForEachWorker<TSource, TLocal>(TSource[] array, ParallelOptions parallelOptions, Action<TSource> body, Action<TSource, ParallelLoopState> bodyWithState, Action<TSource, ParallelLoopState, long> bodyWithStateAndIndex, Func<TSource, ParallelLoopState, TLocal, TLocal> bodyWithStateAndLocal, Func<TSource, ParallelLoopState, long, TLocal, TLocal> bodyWithEverything, Func<TLocal> localInit, Action<TLocal> localFinally)
	{
		int lowerBound = array.GetLowerBound(0);
		int toExclusive = array.GetUpperBound(0) + 1;
		if (body != null)
		{
			return ForWorker<object>(lowerBound, toExclusive, parallelOptions, delegate(int i)
			{
				body(array[i]);
			}, null, null, null, null);
		}
		if (bodyWithState != null)
		{
			return ForWorker<object>(lowerBound, toExclusive, parallelOptions, null, delegate(int i, ParallelLoopState state)
			{
				bodyWithState(array[i], state);
			}, null, null, null);
		}
		if (bodyWithStateAndIndex != null)
		{
			return ForWorker<object>(lowerBound, toExclusive, parallelOptions, null, delegate(int i, ParallelLoopState state)
			{
				bodyWithStateAndIndex(array[i], state, i);
			}, null, null, null);
		}
		if (bodyWithStateAndLocal != null)
		{
			return ForWorker(lowerBound, toExclusive, parallelOptions, null, null, (int i, ParallelLoopState state, TLocal local) => bodyWithStateAndLocal(array[i], state, local), localInit, localFinally);
		}
		return ForWorker(lowerBound, toExclusive, parallelOptions, null, null, (int i, ParallelLoopState state, TLocal local) => bodyWithEverything(array[i], state, i, local), localInit, localFinally);
	}

	private static ParallelLoopResult ForEachWorker<TSource, TLocal>(IList<TSource> list, ParallelOptions parallelOptions, Action<TSource> body, Action<TSource, ParallelLoopState> bodyWithState, Action<TSource, ParallelLoopState, long> bodyWithStateAndIndex, Func<TSource, ParallelLoopState, TLocal, TLocal> bodyWithStateAndLocal, Func<TSource, ParallelLoopState, long, TLocal, TLocal> bodyWithEverything, Func<TLocal> localInit, Action<TLocal> localFinally)
	{
		if (body != null)
		{
			return ForWorker<object>(0, list.Count, parallelOptions, delegate(int i)
			{
				body(list[i]);
			}, null, null, null, null);
		}
		if (bodyWithState != null)
		{
			return ForWorker<object>(0, list.Count, parallelOptions, null, delegate(int i, ParallelLoopState state)
			{
				bodyWithState(list[i], state);
			}, null, null, null);
		}
		if (bodyWithStateAndIndex != null)
		{
			return ForWorker<object>(0, list.Count, parallelOptions, null, delegate(int i, ParallelLoopState state)
			{
				bodyWithStateAndIndex(list[i], state, i);
			}, null, null, null);
		}
		if (bodyWithStateAndLocal != null)
		{
			return ForWorker(0, list.Count, parallelOptions, null, null, (int i, ParallelLoopState state, TLocal local) => bodyWithStateAndLocal(list[i], state, local), localInit, localFinally);
		}
		return ForWorker(0, list.Count, parallelOptions, null, null, (int i, ParallelLoopState state, TLocal local) => bodyWithEverything(list[i], state, i, local), localInit, localFinally);
	}

	public static ParallelLoopResult ForEach<TSource>(Partitioner<TSource> source, Action<TSource> body)
	{
		if (source == null)
		{
			throw new ArgumentNullException("source");
		}
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		return PartitionerForEachWorker<TSource, object>(source, s_defaultParallelOptions, body, null, null, null, null, null, null);
	}

	public static ParallelLoopResult ForEach<TSource>(Partitioner<TSource> source, Action<TSource, ParallelLoopState> body)
	{
		if (source == null)
		{
			throw new ArgumentNullException("source");
		}
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		return PartitionerForEachWorker<TSource, object>(source, s_defaultParallelOptions, null, body, null, null, null, null, null);
	}

	public static ParallelLoopResult ForEach<TSource>(OrderablePartitioner<TSource> source, Action<TSource, ParallelLoopState, long> body)
	{
		if (source == null)
		{
			throw new ArgumentNullException("source");
		}
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		if (!source.KeysNormalized)
		{
			throw new InvalidOperationException(Environment.GetResourceString("This method requires the use of an OrderedPartitioner with the KeysNormalized property set to true."));
		}
		return PartitionerForEachWorker<TSource, object>(source, s_defaultParallelOptions, null, null, body, null, null, null, null);
	}

	public static ParallelLoopResult ForEach<TSource, TLocal>(Partitioner<TSource> source, Func<TLocal> localInit, Func<TSource, ParallelLoopState, TLocal, TLocal> body, Action<TLocal> localFinally)
	{
		if (source == null)
		{
			throw new ArgumentNullException("source");
		}
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		if (localInit == null)
		{
			throw new ArgumentNullException("localInit");
		}
		if (localFinally == null)
		{
			throw new ArgumentNullException("localFinally");
		}
		return PartitionerForEachWorker(source, s_defaultParallelOptions, null, null, null, body, null, localInit, localFinally);
	}

	public static ParallelLoopResult ForEach<TSource, TLocal>(OrderablePartitioner<TSource> source, Func<TLocal> localInit, Func<TSource, ParallelLoopState, long, TLocal, TLocal> body, Action<TLocal> localFinally)
	{
		if (source == null)
		{
			throw new ArgumentNullException("source");
		}
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		if (localInit == null)
		{
			throw new ArgumentNullException("localInit");
		}
		if (localFinally == null)
		{
			throw new ArgumentNullException("localFinally");
		}
		if (!source.KeysNormalized)
		{
			throw new InvalidOperationException(Environment.GetResourceString("This method requires the use of an OrderedPartitioner with the KeysNormalized property set to true."));
		}
		return PartitionerForEachWorker(source, s_defaultParallelOptions, null, null, null, null, body, localInit, localFinally);
	}

	public static ParallelLoopResult ForEach<TSource>(Partitioner<TSource> source, ParallelOptions parallelOptions, Action<TSource> body)
	{
		if (source == null)
		{
			throw new ArgumentNullException("source");
		}
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		if (parallelOptions == null)
		{
			throw new ArgumentNullException("parallelOptions");
		}
		return PartitionerForEachWorker<TSource, object>(source, parallelOptions, body, null, null, null, null, null, null);
	}

	public static ParallelLoopResult ForEach<TSource>(Partitioner<TSource> source, ParallelOptions parallelOptions, Action<TSource, ParallelLoopState> body)
	{
		if (source == null)
		{
			throw new ArgumentNullException("source");
		}
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		if (parallelOptions == null)
		{
			throw new ArgumentNullException("parallelOptions");
		}
		return PartitionerForEachWorker<TSource, object>(source, parallelOptions, null, body, null, null, null, null, null);
	}

	public static ParallelLoopResult ForEach<TSource>(OrderablePartitioner<TSource> source, ParallelOptions parallelOptions, Action<TSource, ParallelLoopState, long> body)
	{
		if (source == null)
		{
			throw new ArgumentNullException("source");
		}
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		if (parallelOptions == null)
		{
			throw new ArgumentNullException("parallelOptions");
		}
		if (!source.KeysNormalized)
		{
			throw new InvalidOperationException(Environment.GetResourceString("This method requires the use of an OrderedPartitioner with the KeysNormalized property set to true."));
		}
		return PartitionerForEachWorker<TSource, object>(source, parallelOptions, null, null, body, null, null, null, null);
	}

	public static ParallelLoopResult ForEach<TSource, TLocal>(Partitioner<TSource> source, ParallelOptions parallelOptions, Func<TLocal> localInit, Func<TSource, ParallelLoopState, TLocal, TLocal> body, Action<TLocal> localFinally)
	{
		if (source == null)
		{
			throw new ArgumentNullException("source");
		}
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		if (localInit == null)
		{
			throw new ArgumentNullException("localInit");
		}
		if (localFinally == null)
		{
			throw new ArgumentNullException("localFinally");
		}
		if (parallelOptions == null)
		{
			throw new ArgumentNullException("parallelOptions");
		}
		return PartitionerForEachWorker(source, parallelOptions, null, null, null, body, null, localInit, localFinally);
	}

	public static ParallelLoopResult ForEach<TSource, TLocal>(OrderablePartitioner<TSource> source, ParallelOptions parallelOptions, Func<TLocal> localInit, Func<TSource, ParallelLoopState, long, TLocal, TLocal> body, Action<TLocal> localFinally)
	{
		if (source == null)
		{
			throw new ArgumentNullException("source");
		}
		if (body == null)
		{
			throw new ArgumentNullException("body");
		}
		if (localInit == null)
		{
			throw new ArgumentNullException("localInit");
		}
		if (localFinally == null)
		{
			throw new ArgumentNullException("localFinally");
		}
		if (parallelOptions == null)
		{
			throw new ArgumentNullException("parallelOptions");
		}
		if (!source.KeysNormalized)
		{
			throw new InvalidOperationException(Environment.GetResourceString("This method requires the use of an OrderedPartitioner with the KeysNormalized property set to true."));
		}
		return PartitionerForEachWorker(source, parallelOptions, null, null, null, null, body, localInit, localFinally);
	}

	private static ParallelLoopResult PartitionerForEachWorker<TSource, TLocal>(Partitioner<TSource> source, ParallelOptions parallelOptions, Action<TSource> simpleBody, Action<TSource, ParallelLoopState> bodyWithState, Action<TSource, ParallelLoopState, long> bodyWithStateAndIndex, Func<TSource, ParallelLoopState, TLocal, TLocal> bodyWithStateAndLocal, Func<TSource, ParallelLoopState, long, TLocal, TLocal> bodyWithEverything, Func<TLocal> localInit, Action<TLocal> localFinally)
	{
		OrderablePartitioner<TSource> orderedSource = source as OrderablePartitioner<TSource>;
		if (!source.SupportsDynamicPartitions)
		{
			throw new InvalidOperationException(Environment.GetResourceString("The Partitioner used here must support dynamic partitioning."));
		}
		if (parallelOptions.CancellationToken.IsCancellationRequested)
		{
			throw new OperationCanceledException(parallelOptions.CancellationToken);
		}
		ParallelLoopStateFlags64 sharedPStateFlags = new ParallelLoopStateFlags64();
		ParallelLoopResult result = default(ParallelLoopResult);
		OperationCanceledException oce = null;
		CancellationTokenRegistration cancellationTokenRegistration = default(CancellationTokenRegistration);
		if (parallelOptions.CancellationToken.CanBeCanceled)
		{
			cancellationTokenRegistration = parallelOptions.CancellationToken.InternalRegisterWithoutEC(delegate
			{
				sharedPStateFlags.Cancel();
				oce = new OperationCanceledException(parallelOptions.CancellationToken);
			}, null);
		}
		IEnumerable<TSource> partitionerSource = null;
		IEnumerable<KeyValuePair<long, TSource>> orderablePartitionerSource = null;
		if (orderedSource != null)
		{
			orderablePartitionerSource = orderedSource.GetOrderableDynamicPartitions();
			if (orderablePartitionerSource == null)
			{
				throw new InvalidOperationException(Environment.GetResourceString("The Partitioner used here returned a null partitioner source."));
			}
		}
		else
		{
			partitionerSource = source.GetDynamicPartitions();
			if (partitionerSource == null)
			{
				throw new InvalidOperationException(Environment.GetResourceString("The Partitioner used here returned a null partitioner source."));
			}
		}
		ParallelForReplicatingTask rootTask = null;
		Action action = delegate
		{
			Task internalCurrent = Task.InternalCurrent;
			TLocal val = default(TLocal);
			bool flag = false;
			IDisposable disposable2 = null;
			try
			{
				ParallelLoopState64 parallelLoopState = null;
				if (bodyWithState != null || bodyWithStateAndIndex != null)
				{
					parallelLoopState = new ParallelLoopState64(sharedPStateFlags);
				}
				else if (bodyWithStateAndLocal != null || bodyWithEverything != null)
				{
					parallelLoopState = new ParallelLoopState64(sharedPStateFlags);
					if (localInit != null)
					{
						val = localInit();
						flag = true;
					}
				}
				bool flag2 = rootTask == internalCurrent;
				LoopTimer loopTimer = new LoopTimer(rootTask.ActiveChildCount);
				if (orderedSource != null)
				{
					IEnumerator<KeyValuePair<long, TSource>> enumerator = internalCurrent.SavedStateFromPreviousReplica as IEnumerator<KeyValuePair<long, TSource>>;
					if (enumerator == null)
					{
						enumerator = orderablePartitionerSource.GetEnumerator();
						if (enumerator == null)
						{
							throw new InvalidOperationException(Environment.GetResourceString("The Partitioner source returned a null enumerator."));
						}
					}
					disposable2 = enumerator;
					while (enumerator.MoveNext())
					{
						KeyValuePair<long, TSource> current = enumerator.Current;
						long key = current.Key;
						TSource value = current.Value;
						if (parallelLoopState != null)
						{
							parallelLoopState.CurrentIteration = key;
						}
						if (simpleBody != null)
						{
							simpleBody(value);
						}
						else if (bodyWithState != null)
						{
							bodyWithState(value, parallelLoopState);
						}
						else if (bodyWithStateAndIndex == null)
						{
							val = ((bodyWithStateAndLocal == null) ? bodyWithEverything(value, parallelLoopState, key, val) : bodyWithStateAndLocal(value, parallelLoopState, val));
						}
						else
						{
							bodyWithStateAndIndex(value, parallelLoopState, key);
						}
						if (sharedPStateFlags.ShouldExitLoop(key))
						{
							break;
						}
						if (!flag2 && loopTimer.LimitExceeded())
						{
							internalCurrent.SavedStateForNextReplica = enumerator;
							disposable2 = null;
							break;
						}
					}
				}
				else
				{
					IEnumerator<TSource> enumerator2 = internalCurrent.SavedStateFromPreviousReplica as IEnumerator<TSource>;
					if (enumerator2 == null)
					{
						enumerator2 = partitionerSource.GetEnumerator();
						if (enumerator2 == null)
						{
							throw new InvalidOperationException(Environment.GetResourceString("The Partitioner source returned a null enumerator."));
						}
					}
					disposable2 = enumerator2;
					if (parallelLoopState != null)
					{
						parallelLoopState.CurrentIteration = 0L;
					}
					while (enumerator2.MoveNext())
					{
						TSource current2 = enumerator2.Current;
						if (simpleBody != null)
						{
							simpleBody(current2);
						}
						else if (bodyWithState != null)
						{
							bodyWithState(current2, parallelLoopState);
						}
						else if (bodyWithStateAndLocal != null)
						{
							val = bodyWithStateAndLocal(current2, parallelLoopState, val);
						}
						if (sharedPStateFlags.LoopStateFlags != ParallelLoopStateFlags.PLS_NONE)
						{
							break;
						}
						if (!flag2 && loopTimer.LimitExceeded())
						{
							internalCurrent.SavedStateForNextReplica = enumerator2;
							disposable2 = null;
							break;
						}
					}
				}
			}
			catch
			{
				sharedPStateFlags.SetExceptional();
				throw;
			}
			finally
			{
				if (localFinally != null && flag)
				{
					localFinally(val);
				}
				disposable2?.Dispose();
			}
		};
		try
		{
			rootTask = new ParallelForReplicatingTask(parallelOptions, action, TaskCreationOptions.None, InternalTaskOptions.SelfReplicating);
			rootTask.RunSynchronously(parallelOptions.EffectiveTaskScheduler);
			rootTask.Wait();
			if (parallelOptions.CancellationToken.CanBeCanceled)
			{
				cancellationTokenRegistration.Dispose();
			}
			if (oce != null)
			{
				throw oce;
			}
		}
		catch (AggregateException ex)
		{
			if (parallelOptions.CancellationToken.CanBeCanceled)
			{
				cancellationTokenRegistration.Dispose();
			}
			ThrowIfReducableToSingleOCE(ex.InnerExceptions, parallelOptions.CancellationToken);
			throw;
		}
		catch (TaskSchedulerException)
		{
			if (parallelOptions.CancellationToken.CanBeCanceled)
			{
				cancellationTokenRegistration.Dispose();
			}
			throw;
		}
		finally
		{
			int loopStateFlags = sharedPStateFlags.LoopStateFlags;
			result.m_completed = loopStateFlags == ParallelLoopStateFlags.PLS_NONE;
			if ((loopStateFlags & ParallelLoopStateFlags.PLS_BROKEN) != 0)
			{
				result.m_lowestBreakIteration = sharedPStateFlags.LowestBreakIteration;
			}
			if (rootTask != null && rootTask.IsCompleted)
			{
				rootTask.Dispose();
			}
			IDisposable disposable = null;
			((orderablePartitionerSource == null) ? (partitionerSource as IDisposable) : (orderablePartitionerSource as IDisposable))?.Dispose();
		}
		return result;
	}

	internal static void ThrowIfReducableToSingleOCE(IEnumerable<Exception> excCollection, CancellationToken ct)
	{
		bool flag = false;
		if (!ct.IsCancellationRequested)
		{
			return;
		}
		foreach (Exception item in excCollection)
		{
			flag = true;
			if (!(item is OperationCanceledException ex) || ex.CancellationToken != ct)
			{
				return;
			}
		}
		if (flag)
		{
			throw new OperationCanceledException(ct);
		}
	}
}

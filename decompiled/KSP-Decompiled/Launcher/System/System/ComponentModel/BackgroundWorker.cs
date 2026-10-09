using System.Security.Permissions;
using System.Threading;

namespace System.ComponentModel;

[SRDescription("Executes an operation on a separate thread.")]
[DefaultEvent("DoWork")]
[HostProtection(SecurityAction.LinkDemand, SharedState = true)]
public class BackgroundWorker : Component
{
	private delegate void WorkerThreadStartDelegate(object argument);

	private static readonly object doWorkKey = new object();

	private static readonly object runWorkerCompletedKey = new object();

	private static readonly object progressChangedKey = new object();

	private bool canCancelWorker;

	private bool workerReportsProgress;

	private bool cancellationPending;

	private bool isRunning;

	private AsyncOperation asyncOperation;

	private readonly WorkerThreadStartDelegate threadStart;

	private readonly SendOrPostCallback operationCompleted;

	private readonly SendOrPostCallback progressReporter;

	[Browsable(false)]
	[SRDescription("Has the user attempted to cancel the operation? To be accessed from DoWork event handler.")]
	public bool CancellationPending => cancellationPending;

	[SRDescription("Is the worker still currently working on a background operation?")]
	[Browsable(false)]
	public bool IsBusy => isRunning;

	[SRDescription("Whether the worker will report progress.")]
	[SRCategory("Asynchronous")]
	[DefaultValue(false)]
	public bool WorkerReportsProgress
	{
		get
		{
			return workerReportsProgress;
		}
		set
		{
			workerReportsProgress = value;
		}
	}

	[SRCategory("Asynchronous")]
	[SRDescription("Whether the worker supports cancellation.")]
	[DefaultValue(false)]
	public bool WorkerSupportsCancellation
	{
		get
		{
			return canCancelWorker;
		}
		set
		{
			canCancelWorker = value;
		}
	}

	[SRCategory("Asynchronous")]
	[SRDescription("Event handler to be run on a different thread when the operation begins.")]
	public event DoWorkEventHandler DoWork
	{
		add
		{
			base.Events.AddHandler(doWorkKey, value);
		}
		remove
		{
			base.Events.RemoveHandler(doWorkKey, value);
		}
	}

	[SRCategory("Asynchronous")]
	[SRDescription("Raised when the worker thread indicates that some progress has been made.")]
	public event ProgressChangedEventHandler ProgressChanged
	{
		add
		{
			base.Events.AddHandler(progressChangedKey, value);
		}
		remove
		{
			base.Events.RemoveHandler(progressChangedKey, value);
		}
	}

	[SRDescription("Raised when the worker has completed (either through success, failure, or cancellation).")]
	[SRCategory("Asynchronous")]
	public event RunWorkerCompletedEventHandler RunWorkerCompleted
	{
		add
		{
			base.Events.AddHandler(runWorkerCompletedKey, value);
		}
		remove
		{
			base.Events.RemoveHandler(runWorkerCompletedKey, value);
		}
	}

	public BackgroundWorker()
	{
		threadStart = WorkerThreadStart;
		operationCompleted = AsyncOperationCompleted;
		progressReporter = ProgressReporter;
	}

	private void AsyncOperationCompleted(object arg)
	{
		isRunning = false;
		cancellationPending = false;
		OnRunWorkerCompleted((RunWorkerCompletedEventArgs)arg);
	}

	public void CancelAsync()
	{
		if (!WorkerSupportsCancellation)
		{
			throw new InvalidOperationException(global::SR.GetString("This BackgroundWorker states that it doesn't support cancellation. Modify WorkerSupportsCancellation to state that it does support cancellation."));
		}
		cancellationPending = true;
	}

	protected virtual void OnDoWork(DoWorkEventArgs e)
	{
		((DoWorkEventHandler)base.Events[doWorkKey])?.Invoke(this, e);
	}

	protected virtual void OnRunWorkerCompleted(RunWorkerCompletedEventArgs e)
	{
		((RunWorkerCompletedEventHandler)base.Events[runWorkerCompletedKey])?.Invoke(this, e);
	}

	protected virtual void OnProgressChanged(ProgressChangedEventArgs e)
	{
		((ProgressChangedEventHandler)base.Events[progressChangedKey])?.Invoke(this, e);
	}

	private void ProgressReporter(object arg)
	{
		OnProgressChanged((ProgressChangedEventArgs)arg);
	}

	public void ReportProgress(int percentProgress)
	{
		ReportProgress(percentProgress, null);
	}

	public void ReportProgress(int percentProgress, object userState)
	{
		if (!WorkerReportsProgress)
		{
			throw new InvalidOperationException(global::SR.GetString("This BackgroundWorker states that it doesn't report progress. Modify WorkerReportsProgress to state that it does report progress."));
		}
		ProgressChangedEventArgs progressChangedEventArgs = new ProgressChangedEventArgs(percentProgress, userState);
		if (asyncOperation != null)
		{
			asyncOperation.Post(progressReporter, progressChangedEventArgs);
		}
		else
		{
			progressReporter(progressChangedEventArgs);
		}
	}

	public void RunWorkerAsync()
	{
		RunWorkerAsync(null);
	}

	public void RunWorkerAsync(object argument)
	{
		if (isRunning)
		{
			throw new InvalidOperationException(global::SR.GetString("This BackgroundWorker is currently busy and cannot run multiple tasks concurrently."));
		}
		isRunning = true;
		cancellationPending = false;
		asyncOperation = AsyncOperationManager.CreateOperation(null);
		threadStart.BeginInvoke(argument, null, null);
	}

	private void WorkerThreadStart(object argument)
	{
		object result = null;
		Exception error = null;
		bool cancelled = false;
		try
		{
			DoWorkEventArgs doWorkEventArgs = new DoWorkEventArgs(argument);
			OnDoWork(doWorkEventArgs);
			if (doWorkEventArgs.Cancel)
			{
				cancelled = true;
			}
			else
			{
				result = doWorkEventArgs.Result;
			}
		}
		catch (Exception ex)
		{
			error = ex;
		}
		RunWorkerCompletedEventArgs arg = new RunWorkerCompletedEventArgs(result, error, cancelled);
		asyncOperation.PostOperationCompleted(operationCompleted, arg);
	}
}

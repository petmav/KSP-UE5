using System.Threading;

namespace System;

public class Progress<T> : IProgress<T>
{
	private readonly SynchronizationContext m_synchronizationContext;

	private readonly Action<T> m_handler;

	private readonly SendOrPostCallback m_invokeHandlers;

	public event EventHandler<T> ProgressChanged;

	public Progress()
	{
		m_synchronizationContext = SynchronizationContext.CurrentNoFlow ?? ProgressStatics.DefaultContext;
		m_invokeHandlers = InvokeHandlers;
	}

	public Progress(Action<T> handler)
		: this()
	{
		if (handler == null)
		{
			throw new ArgumentNullException("handler");
		}
		m_handler = handler;
	}

	protected virtual void OnReport(T value)
	{
		Action<T> handler = m_handler;
		EventHandler<T> progressChanged = this.ProgressChanged;
		if (handler != null || progressChanged != null)
		{
			m_synchronizationContext.Post(m_invokeHandlers, value);
		}
	}

	void IProgress<T>.Report(T value)
	{
		OnReport(value);
	}

	private void InvokeHandlers(object state)
	{
		T val = (T)state;
		Action<T> handler = m_handler;
		EventHandler<T> progressChanged = this.ProgressChanged;
		handler?.Invoke(val);
		progressChanged?.Invoke(this, val);
	}
}

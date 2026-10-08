using System.Diagnostics;
using System.Runtime.ExceptionServices;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Security.Permissions;
using System.Threading;

namespace System;

[Serializable]
[DebuggerTypeProxy(typeof(System_LazyDebugView<>))]
[ComVisible(false)]
[DebuggerDisplay("ThreadSafetyMode={Mode}, IsValueCreated={IsValueCreated}, IsValueFaulted={IsValueFaulted}, Value={ValueForDebugDisplay}")]
[HostProtection(SecurityAction.LinkDemand, Synchronization = true, ExternalThreading = true)]
public class Lazy<T>
{
	[Serializable]
	private class Boxed
	{
		internal T m_value;

		internal Boxed(T value)
		{
			m_value = value;
		}
	}

	private class LazyInternalExceptionHolder
	{
		internal ExceptionDispatchInfo m_edi;

		internal LazyInternalExceptionHolder(Exception ex)
		{
			m_edi = ExceptionDispatchInfo.Capture(ex);
		}
	}

	private static readonly Func<T> ALREADY_INVOKED_SENTINEL = () => default(T);

	private object m_boxed;

	[NonSerialized]
	private Func<T> m_valueFactory;

	[NonSerialized]
	private object m_threadSafeObj;

	internal T ValueForDebugDisplay
	{
		get
		{
			if (!IsValueCreated)
			{
				return default(T);
			}
			return ((Boxed)m_boxed).m_value;
		}
	}

	internal LazyThreadSafetyMode Mode
	{
		get
		{
			if (m_threadSafeObj == null)
			{
				return LazyThreadSafetyMode.None;
			}
			if (m_threadSafeObj == LazyHelpers.PUBLICATION_ONLY_SENTINEL)
			{
				return LazyThreadSafetyMode.PublicationOnly;
			}
			return LazyThreadSafetyMode.ExecutionAndPublication;
		}
	}

	internal bool IsValueFaulted => m_boxed is LazyInternalExceptionHolder;

	public bool IsValueCreated
	{
		get
		{
			if (m_boxed != null)
			{
				return m_boxed is Boxed;
			}
			return false;
		}
	}

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	public T Value
	{
		get
		{
			Boxed boxed = null;
			if (m_boxed != null)
			{
				if (m_boxed is Boxed boxed2)
				{
					return boxed2.m_value;
				}
				(m_boxed as LazyInternalExceptionHolder).m_edi.Throw();
			}
			Debugger.NotifyOfCrossThreadDependency();
			return LazyInitValue();
		}
	}

	public Lazy()
		: this(LazyThreadSafetyMode.ExecutionAndPublication)
	{
	}

	public Lazy(Func<T> valueFactory)
		: this(valueFactory, LazyThreadSafetyMode.ExecutionAndPublication)
	{
	}

	public Lazy(bool isThreadSafe)
		: this(isThreadSafe ? LazyThreadSafetyMode.ExecutionAndPublication : LazyThreadSafetyMode.None)
	{
	}

	public Lazy(LazyThreadSafetyMode mode)
	{
		m_threadSafeObj = GetObjectFromMode(mode);
	}

	public Lazy(Func<T> valueFactory, bool isThreadSafe)
		: this(valueFactory, isThreadSafe ? LazyThreadSafetyMode.ExecutionAndPublication : LazyThreadSafetyMode.None)
	{
	}

	public Lazy(Func<T> valueFactory, LazyThreadSafetyMode mode)
	{
		if (valueFactory == null)
		{
			throw new ArgumentNullException("valueFactory");
		}
		m_threadSafeObj = GetObjectFromMode(mode);
		m_valueFactory = valueFactory;
	}

	private static object GetObjectFromMode(LazyThreadSafetyMode mode)
	{
		return mode switch
		{
			LazyThreadSafetyMode.ExecutionAndPublication => new object(), 
			LazyThreadSafetyMode.PublicationOnly => LazyHelpers.PUBLICATION_ONLY_SENTINEL, 
			LazyThreadSafetyMode.None => null, 
			_ => throw new ArgumentOutOfRangeException("mode", Environment.GetResourceString("The mode argument specifies an invalid value.")), 
		};
	}

	[OnSerializing]
	private void OnSerializing(StreamingContext context)
	{
		_ = Value;
	}

	public override string ToString()
	{
		if (!IsValueCreated)
		{
			return Environment.GetResourceString("Value is not created.");
		}
		return Value.ToString();
	}

	private T LazyInitValue()
	{
		Boxed boxed = null;
		switch (Mode)
		{
		case LazyThreadSafetyMode.None:
			boxed = (Boxed)(m_boxed = CreateValue());
			break;
		case LazyThreadSafetyMode.PublicationOnly:
			boxed = CreateValue();
			if (boxed == null || Interlocked.CompareExchange(ref m_boxed, boxed, null) != null)
			{
				boxed = (Boxed)m_boxed;
			}
			else
			{
				m_valueFactory = ALREADY_INVOKED_SENTINEL;
			}
			break;
		default:
		{
			object obj = Volatile.Read(ref m_threadSafeObj);
			bool lockTaken = false;
			try
			{
				if (obj != ALREADY_INVOKED_SENTINEL)
				{
					Monitor.Enter(obj, ref lockTaken);
				}
				if (m_boxed == null)
				{
					boxed = (Boxed)(m_boxed = CreateValue());
					Volatile.Write(ref m_threadSafeObj, ALREADY_INVOKED_SENTINEL);
					break;
				}
				boxed = m_boxed as Boxed;
				if (boxed == null)
				{
					(m_boxed as LazyInternalExceptionHolder).m_edi.Throw();
				}
			}
			finally
			{
				if (lockTaken)
				{
					Monitor.Exit(obj);
				}
			}
			break;
		}
		}
		return boxed.m_value;
	}

	private Boxed CreateValue()
	{
		Boxed boxed = null;
		LazyThreadSafetyMode mode = Mode;
		if (m_valueFactory != null)
		{
			try
			{
				if (mode != LazyThreadSafetyMode.PublicationOnly && m_valueFactory == ALREADY_INVOKED_SENTINEL)
				{
					throw new InvalidOperationException(Environment.GetResourceString("ValueFactory attempted to access the Value property of this instance."));
				}
				Func<T> valueFactory = m_valueFactory;
				if (mode != LazyThreadSafetyMode.PublicationOnly)
				{
					m_valueFactory = ALREADY_INVOKED_SENTINEL;
				}
				else if (valueFactory == ALREADY_INVOKED_SENTINEL)
				{
					return null;
				}
				return new Boxed(valueFactory());
			}
			catch (Exception ex)
			{
				if (mode != LazyThreadSafetyMode.PublicationOnly)
				{
					m_boxed = new LazyInternalExceptionHolder(ex);
				}
				throw;
			}
		}
		try
		{
			return new Boxed((T)Activator.CreateInstance(typeof(T)));
		}
		catch (MissingMethodException)
		{
			Exception ex2 = new MissingMemberException(Environment.GetResourceString("The lazily-initialized type does not have a public, parameterless constructor."));
			if (mode != LazyThreadSafetyMode.PublicationOnly)
			{
				m_boxed = new LazyInternalExceptionHolder(ex2);
			}
			throw ex2;
		}
	}
}

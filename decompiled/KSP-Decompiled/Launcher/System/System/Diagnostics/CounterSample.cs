using Unity;

namespace System.Diagnostics;

public struct CounterSample
{
	public static CounterSample Empty;

	public long BaseValue
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(long);
		}
	}

	public long CounterFrequency
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(long);
		}
	}

	public long CounterTimeStamp
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(long);
		}
	}

	public PerformanceCounterType CounterType
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(PerformanceCounterType);
		}
	}

	public long RawValue
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(long);
		}
	}

	public long SystemFrequency
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(long);
		}
	}

	public long TimeStamp
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(long);
		}
	}

	public long TimeStamp100nSec
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(long);
		}
	}

	public CounterSample(long rawValue, long baseValue, long counterFrequency, long systemFrequency, long timeStamp, long timeStamp100nSec, PerformanceCounterType counterType)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CounterSample(long rawValue, long baseValue, long counterFrequency, long systemFrequency, long timeStamp, long timeStamp100nSec, PerformanceCounterType counterType, long counterTimeStamp)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static float Calculate(CounterSample counterSample)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(float);
	}

	public static float Calculate(CounterSample counterSample, CounterSample nextCounterSample)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(float);
	}

	public bool Equals(CounterSample sample)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public static bool operator ==(CounterSample a, CounterSample b)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public static bool operator !=(CounterSample a, CounterSample b)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}
}

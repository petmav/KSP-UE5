namespace System.Diagnostics;

public struct CounterSample
{
	private long rawValue;

	private long baseValue;

	private long counterFrequency;

	private long systemFrequency;

	private long timeStamp;

	private long timeStamp100nSec;

	private long counterTimeStamp;

	private PerformanceCounterType counterType;

	public static CounterSample Empty = new CounterSample(0L, 0L, 0L, 0L, 0L, 0L, PerformanceCounterType.NumberOfItems32, 0L);

	public long BaseValue => baseValue;

	public long CounterFrequency => counterFrequency;

	public long CounterTimeStamp => counterTimeStamp;

	public PerformanceCounterType CounterType => counterType;

	public long RawValue => rawValue;

	public long SystemFrequency => systemFrequency;

	public long TimeStamp => timeStamp;

	public long TimeStamp100nSec => timeStamp100nSec;

	public CounterSample(long rawValue, long baseValue, long counterFrequency, long systemFrequency, long timeStamp, long timeStamp100nSec, PerformanceCounterType counterType)
		: this(rawValue, baseValue, counterFrequency, systemFrequency, timeStamp, timeStamp100nSec, counterType, 0L)
	{
	}

	public CounterSample(long rawValue, long baseValue, long counterFrequency, long systemFrequency, long timeStamp, long timeStamp100nSec, PerformanceCounterType counterType, long counterTimeStamp)
	{
		this.rawValue = rawValue;
		this.baseValue = baseValue;
		this.counterFrequency = counterFrequency;
		this.systemFrequency = systemFrequency;
		this.timeStamp = timeStamp;
		this.timeStamp100nSec = timeStamp100nSec;
		this.counterType = counterType;
		this.counterTimeStamp = counterTimeStamp;
	}

	public static float Calculate(CounterSample counterSample)
	{
		return CounterSampleCalculator.ComputeCounterValue(counterSample);
	}

	public static float Calculate(CounterSample counterSample, CounterSample nextCounterSample)
	{
		return CounterSampleCalculator.ComputeCounterValue(counterSample, nextCounterSample);
	}

	public override bool Equals(object o)
	{
		if (!(o is CounterSample))
		{
			return false;
		}
		return Equals((CounterSample)o);
	}

	public bool Equals(CounterSample sample)
	{
		if (rawValue == sample.rawValue && baseValue == sample.counterFrequency && counterFrequency == sample.counterFrequency && systemFrequency == sample.systemFrequency && timeStamp == sample.timeStamp && timeStamp100nSec == sample.timeStamp100nSec && counterTimeStamp == sample.counterTimeStamp)
		{
			return counterType == sample.counterType;
		}
		return false;
	}

	public static bool operator ==(CounterSample a, CounterSample b)
	{
		return a.Equals(b);
	}

	public static bool operator !=(CounterSample a, CounterSample b)
	{
		return !a.Equals(b);
	}

	public override int GetHashCode()
	{
		return (int)((rawValue << 28) ^ ((baseValue << 24) ^ ((counterFrequency << 20) ^ ((systemFrequency << 16) ^ ((timeStamp << 8) ^ ((timeStamp100nSec << 4) ^ (counterTimeStamp ^ (long)counterType)))))));
	}
}

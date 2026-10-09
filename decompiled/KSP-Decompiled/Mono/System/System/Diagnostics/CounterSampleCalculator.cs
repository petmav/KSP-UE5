namespace System.Diagnostics;

public static class CounterSampleCalculator
{
	public static float ComputeCounterValue(CounterSample newSample)
	{
		switch (newSample.CounterType)
		{
		case PerformanceCounterType.NumberOfItemsHEX32:
		case PerformanceCounterType.NumberOfItemsHEX64:
		case PerformanceCounterType.NumberOfItems32:
		case PerformanceCounterType.NumberOfItems64:
		case PerformanceCounterType.RawFraction:
			return newSample.RawValue;
		default:
			return 0f;
		}
	}

	[System.MonoTODO("What's the algorithm?")]
	public static float ComputeCounterValue(CounterSample oldSample, CounterSample newSample)
	{
		if (newSample.CounterType != oldSample.CounterType)
		{
			throw new Exception("The counter samples must be of the same type");
		}
		switch (newSample.CounterType)
		{
		case PerformanceCounterType.NumberOfItemsHEX32:
		case PerformanceCounterType.NumberOfItemsHEX64:
		case PerformanceCounterType.NumberOfItems32:
		case PerformanceCounterType.NumberOfItems64:
		case PerformanceCounterType.RawFraction:
			return newSample.RawValue;
		case PerformanceCounterType.AverageCount64:
			return (float)(newSample.RawValue - oldSample.RawValue) / (float)(newSample.BaseValue - oldSample.BaseValue);
		case PerformanceCounterType.AverageTimer32:
			return (float)(newSample.RawValue - oldSample.RawValue) / (float)newSample.SystemFrequency / (float)(newSample.BaseValue - oldSample.BaseValue);
		case PerformanceCounterType.CounterDelta32:
		case PerformanceCounterType.CounterDelta64:
			return newSample.RawValue - oldSample.RawValue;
		case PerformanceCounterType.CounterMultiTimer:
			return (float)(newSample.RawValue - oldSample.RawValue) / (float)(newSample.TimeStamp - oldSample.TimeStamp) * 100f / (float)newSample.BaseValue;
		case PerformanceCounterType.CounterMultiTimer100Ns:
			return (float)(newSample.RawValue - oldSample.RawValue) / (float)(newSample.TimeStamp100nSec - oldSample.TimeStamp100nSec) * 100f / (float)newSample.BaseValue;
		case PerformanceCounterType.CounterMultiTimerInverse:
			return ((float)newSample.BaseValue - (float)(newSample.RawValue - oldSample.RawValue) / (float)(newSample.TimeStamp - oldSample.TimeStamp)) * 100f;
		case PerformanceCounterType.CounterMultiTimer100NsInverse:
			return ((float)newSample.BaseValue - (float)(newSample.RawValue - oldSample.RawValue) / (float)(newSample.TimeStamp100nSec - oldSample.TimeStamp100nSec)) * 100f;
		case PerformanceCounterType.CountPerTimeInterval32:
		case PerformanceCounterType.CountPerTimeInterval64:
		case PerformanceCounterType.CounterTimer:
			return (float)(newSample.RawValue - oldSample.RawValue) / (float)(newSample.TimeStamp - oldSample.TimeStamp);
		case PerformanceCounterType.CounterTimerInverse:
			return (1f - (float)(newSample.RawValue - oldSample.RawValue) / (float)(newSample.TimeStamp100nSec - oldSample.TimeStamp100nSec)) * 100f;
		case PerformanceCounterType.ElapsedTime:
			return 0f;
		case PerformanceCounterType.Timer100Ns:
			return (float)(newSample.RawValue - oldSample.RawValue) / (float)(newSample.TimeStamp - oldSample.TimeStamp) * 100f;
		case PerformanceCounterType.Timer100NsInverse:
			return (1f - (float)(newSample.RawValue - oldSample.RawValue) / (float)(newSample.TimeStamp - oldSample.TimeStamp)) * 100f;
		case PerformanceCounterType.RateOfCountsPerSecond32:
		case PerformanceCounterType.RateOfCountsPerSecond64:
			return (float)(newSample.RawValue - oldSample.RawValue) / (float)(newSample.TimeStamp - oldSample.TimeStamp) * 10000000f;
		default:
			Console.WriteLine("Counter type {0} not handled", newSample.CounterType);
			return 0f;
		}
	}
}

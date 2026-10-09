namespace System.Threading.Tasks;

public class ParallelOptions
{
	private TaskScheduler m_scheduler;

	private int m_maxDegreeOfParallelism;

	private CancellationToken m_cancellationToken;

	public TaskScheduler TaskScheduler
	{
		get
		{
			return m_scheduler;
		}
		set
		{
			m_scheduler = value;
		}
	}

	internal TaskScheduler EffectiveTaskScheduler
	{
		get
		{
			if (m_scheduler == null)
			{
				return TaskScheduler.Current;
			}
			return m_scheduler;
		}
	}

	public int MaxDegreeOfParallelism
	{
		get
		{
			return m_maxDegreeOfParallelism;
		}
		set
		{
			if (value == 0 || value < -1)
			{
				throw new ArgumentOutOfRangeException("MaxDegreeOfParallelism");
			}
			m_maxDegreeOfParallelism = value;
		}
	}

	public CancellationToken CancellationToken
	{
		get
		{
			return m_cancellationToken;
		}
		set
		{
			m_cancellationToken = value;
		}
	}

	internal int EffectiveMaxConcurrencyLevel
	{
		get
		{
			int num = MaxDegreeOfParallelism;
			int maximumConcurrencyLevel = EffectiveTaskScheduler.MaximumConcurrencyLevel;
			if (maximumConcurrencyLevel > 0 && maximumConcurrencyLevel != int.MaxValue)
			{
				num = ((num == -1) ? maximumConcurrencyLevel : Math.Min(maximumConcurrencyLevel, num));
			}
			return num;
		}
	}

	public ParallelOptions()
	{
		m_scheduler = TaskScheduler.Default;
		m_maxDegreeOfParallelism = -1;
		m_cancellationToken = CancellationToken.None;
	}
}

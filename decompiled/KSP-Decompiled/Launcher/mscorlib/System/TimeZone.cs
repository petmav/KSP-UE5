using System.Globalization;
using System.Runtime.InteropServices;

namespace System;

[Serializable]
[ComVisible(true)]
public abstract class TimeZone
{
	private static TimeZone currentTimeZone;

	[NonSerialized]
	private static object tz_lock = new object();

	[NonSerialized]
	private static long timezone_check;

	public static TimeZone CurrentTimeZone
	{
		get
		{
			long ticks = DateTime.UtcNow.Ticks;
			TimeZone timeZone = currentTimeZone;
			lock (tz_lock)
			{
				if (timeZone == null || Math.Abs(ticks - timezone_check) > 600000000)
				{
					timeZone = new CurrentSystemTimeZone();
					timezone_check = ticks;
					currentTimeZone = timeZone;
				}
			}
			return timeZone;
		}
	}

	public abstract string DaylightName { get; }

	public abstract string StandardName { get; }

	public abstract DaylightTime GetDaylightChanges(int year);

	public abstract TimeSpan GetUtcOffset(DateTime time);

	public virtual bool IsDaylightSavingTime(DateTime time)
	{
		return IsDaylightSavingTime(time, GetDaylightChanges(time.Year));
	}

	public static bool IsDaylightSavingTime(DateTime time, DaylightTime daylightTimes)
	{
		if (daylightTimes == null)
		{
			throw new ArgumentNullException("daylightTimes");
		}
		if (daylightTimes.Start.Ticks == daylightTimes.End.Ticks)
		{
			return false;
		}
		if (daylightTimes.Start.Ticks < daylightTimes.End.Ticks)
		{
			if (daylightTimes.Start.Ticks < time.Ticks && daylightTimes.End.Ticks > time.Ticks)
			{
				return true;
			}
		}
		else if (time.Year == daylightTimes.Start.Year && time.Year == daylightTimes.End.Year && (time.Ticks < daylightTimes.End.Ticks || time.Ticks > daylightTimes.Start.Ticks))
		{
			return true;
		}
		return false;
	}

	public virtual DateTime ToLocalTime(DateTime time)
	{
		if (time.Kind == DateTimeKind.Local)
		{
			return time;
		}
		TimeSpan utcOffset = GetUtcOffset(new DateTime(time.Ticks));
		if (utcOffset.Ticks > 0)
		{
			if (DateTime.MaxValue - utcOffset < time)
			{
				return DateTime.SpecifyKind(DateTime.MaxValue, DateTimeKind.Local);
			}
		}
		else if (utcOffset.Ticks < 0 && time.Ticks + utcOffset.Ticks < DateTime.MinValue.Ticks)
		{
			return DateTime.SpecifyKind(DateTime.MinValue, DateTimeKind.Local);
		}
		return DateTime.SpecifyKind(time.Add(utcOffset), DateTimeKind.Local);
	}

	public virtual DateTime ToUniversalTime(DateTime time)
	{
		if (time.Kind == DateTimeKind.Utc)
		{
			return time;
		}
		TimeSpan utcOffset = GetUtcOffset(time);
		if (utcOffset.Ticks < 0)
		{
			if (DateTime.MaxValue + utcOffset < time)
			{
				return DateTime.SpecifyKind(DateTime.MaxValue, DateTimeKind.Utc);
			}
		}
		else if (utcOffset.Ticks > 0 && DateTime.MinValue + utcOffset > time)
		{
			return DateTime.SpecifyKind(DateTime.MinValue, DateTimeKind.Utc);
		}
		return DateTime.SpecifyKind(new DateTime(time.Ticks - utcOffset.Ticks), DateTimeKind.Utc);
	}

	internal static void ClearCachedData()
	{
		currentTimeZone = null;
	}
}

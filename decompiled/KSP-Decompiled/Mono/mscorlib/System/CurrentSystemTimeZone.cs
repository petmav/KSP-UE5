using System.Globalization;
using System.Runtime.CompilerServices;

namespace System;

[Serializable]
internal class CurrentSystemTimeZone : TimeZone
{
	private readonly TimeZoneInfo LocalTimeZone;

	public override string DaylightName => LocalTimeZone.DaylightName;

	public override string StandardName => LocalTimeZone.StandardName;

	internal CurrentSystemTimeZone()
	{
		LocalTimeZone = TimeZoneInfo.Local;
	}

	public override DaylightTime GetDaylightChanges(int year)
	{
		return LocalTimeZone.GetDaylightChanges(year);
	}

	public override TimeSpan GetUtcOffset(DateTime dateTime)
	{
		if (dateTime.Kind == DateTimeKind.Utc)
		{
			return TimeSpan.Zero;
		}
		return LocalTimeZone.GetUtcOffset(dateTime);
	}

	public override bool IsDaylightSavingTime(DateTime dateTime)
	{
		if (dateTime.Kind == DateTimeKind.Utc)
		{
			return false;
		}
		return LocalTimeZone.IsDaylightSavingTime(dateTime);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	public static extern bool GetTimeZoneData(int year, out long[] data, out string[] names, out bool daylight_inverted);
}

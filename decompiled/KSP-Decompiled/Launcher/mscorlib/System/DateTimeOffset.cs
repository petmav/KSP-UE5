using System.Globalization;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Security;

namespace System;

[Serializable]
[StructLayout(LayoutKind.Auto)]
public struct DateTimeOffset : IComparable, IFormattable, ISerializable, IDeserializationCallback, IComparable<DateTimeOffset>, IEquatable<DateTimeOffset>
{
	internal const long MaxOffset = 504000000000L;

	internal const long MinOffset = -504000000000L;

	private const long UnixEpochTicks = 621355968000000000L;

	private const long UnixEpochSeconds = 62135596800L;

	private const long UnixEpochMilliseconds = 62135596800000L;

	public static readonly DateTimeOffset MinValue = new DateTimeOffset(0L, TimeSpan.Zero);

	public static readonly DateTimeOffset MaxValue = new DateTimeOffset(3155378975999999999L, TimeSpan.Zero);

	private DateTime m_dateTime;

	private short m_offsetMinutes;

	public static DateTimeOffset Now => new DateTimeOffset(DateTime.Now);

	public static DateTimeOffset UtcNow => new DateTimeOffset(DateTime.UtcNow);

	public DateTime DateTime => ClockDateTime;

	public DateTime UtcDateTime => DateTime.SpecifyKind(m_dateTime, DateTimeKind.Utc);

	public DateTime LocalDateTime => UtcDateTime.ToLocalTime();

	private DateTime ClockDateTime => new DateTime((m_dateTime + Offset).Ticks, DateTimeKind.Unspecified);

	public DateTime Date => ClockDateTime.Date;

	public int Day => ClockDateTime.Day;

	public DayOfWeek DayOfWeek => ClockDateTime.DayOfWeek;

	public int DayOfYear => ClockDateTime.DayOfYear;

	public int Hour => ClockDateTime.Hour;

	public int Millisecond => ClockDateTime.Millisecond;

	public int Minute => ClockDateTime.Minute;

	public int Month => ClockDateTime.Month;

	public TimeSpan Offset => new TimeSpan(0, m_offsetMinutes, 0);

	public int Second => ClockDateTime.Second;

	public long Ticks => ClockDateTime.Ticks;

	public long UtcTicks => UtcDateTime.Ticks;

	public TimeSpan TimeOfDay => ClockDateTime.TimeOfDay;

	public int Year => ClockDateTime.Year;

	public DateTimeOffset(long ticks, TimeSpan offset)
	{
		m_offsetMinutes = ValidateOffset(offset);
		DateTime dateTime = new DateTime(ticks);
		m_dateTime = ValidateDate(dateTime, offset);
	}

	public DateTimeOffset(DateTime dateTime)
	{
		TimeSpan offset = ((dateTime.Kind == DateTimeKind.Utc) ? new TimeSpan(0L) : TimeZoneInfo.GetLocalUtcOffset(dateTime, TimeZoneInfoOptions.NoThrowOnInvalidTime));
		m_offsetMinutes = ValidateOffset(offset);
		m_dateTime = ValidateDate(dateTime, offset);
	}

	public DateTimeOffset(DateTime dateTime, TimeSpan offset)
	{
		if (dateTime.Kind == DateTimeKind.Local)
		{
			if (offset != TimeZoneInfo.GetLocalUtcOffset(dateTime, TimeZoneInfoOptions.NoThrowOnInvalidTime))
			{
				throw new ArgumentException(Environment.GetResourceString("The UTC Offset of the local dateTime parameter does not match the offset argument."), "offset");
			}
		}
		else if (dateTime.Kind == DateTimeKind.Utc && offset != TimeSpan.Zero)
		{
			throw new ArgumentException(Environment.GetResourceString("The UTC Offset for Utc DateTime instances must be 0."), "offset");
		}
		m_offsetMinutes = ValidateOffset(offset);
		m_dateTime = ValidateDate(dateTime, offset);
	}

	public DateTimeOffset(int year, int month, int day, int hour, int minute, int second, TimeSpan offset)
	{
		m_offsetMinutes = ValidateOffset(offset);
		m_dateTime = ValidateDate(new DateTime(year, month, day, hour, minute, second), offset);
	}

	public DateTimeOffset(int year, int month, int day, int hour, int minute, int second, int millisecond, TimeSpan offset)
	{
		m_offsetMinutes = ValidateOffset(offset);
		m_dateTime = ValidateDate(new DateTime(year, month, day, hour, minute, second, millisecond), offset);
	}

	public DateTimeOffset(int year, int month, int day, int hour, int minute, int second, int millisecond, Calendar calendar, TimeSpan offset)
	{
		m_offsetMinutes = ValidateOffset(offset);
		m_dateTime = ValidateDate(new DateTime(year, month, day, hour, minute, second, millisecond, calendar), offset);
	}

	public DateTimeOffset ToOffset(TimeSpan offset)
	{
		return new DateTimeOffset((m_dateTime + offset).Ticks, offset);
	}

	public DateTimeOffset Add(TimeSpan timeSpan)
	{
		return new DateTimeOffset(ClockDateTime.Add(timeSpan), Offset);
	}

	public DateTimeOffset AddDays(double days)
	{
		return new DateTimeOffset(ClockDateTime.AddDays(days), Offset);
	}

	public DateTimeOffset AddHours(double hours)
	{
		return new DateTimeOffset(ClockDateTime.AddHours(hours), Offset);
	}

	public DateTimeOffset AddMilliseconds(double milliseconds)
	{
		return new DateTimeOffset(ClockDateTime.AddMilliseconds(milliseconds), Offset);
	}

	public DateTimeOffset AddMinutes(double minutes)
	{
		return new DateTimeOffset(ClockDateTime.AddMinutes(minutes), Offset);
	}

	public DateTimeOffset AddMonths(int months)
	{
		return new DateTimeOffset(ClockDateTime.AddMonths(months), Offset);
	}

	public DateTimeOffset AddSeconds(double seconds)
	{
		return new DateTimeOffset(ClockDateTime.AddSeconds(seconds), Offset);
	}

	public DateTimeOffset AddTicks(long ticks)
	{
		return new DateTimeOffset(ClockDateTime.AddTicks(ticks), Offset);
	}

	public DateTimeOffset AddYears(int years)
	{
		return new DateTimeOffset(ClockDateTime.AddYears(years), Offset);
	}

	public static int Compare(DateTimeOffset first, DateTimeOffset second)
	{
		return DateTime.Compare(first.UtcDateTime, second.UtcDateTime);
	}

	int IComparable.CompareTo(object obj)
	{
		if (obj == null)
		{
			return 1;
		}
		if (!(obj is DateTimeOffset { UtcDateTime: var utcDateTime }))
		{
			throw new ArgumentException(Environment.GetResourceString("Object must be of type DateTimeOffset."));
		}
		DateTime utcDateTime2 = UtcDateTime;
		if (utcDateTime2 > utcDateTime)
		{
			return 1;
		}
		if (utcDateTime2 < utcDateTime)
		{
			return -1;
		}
		return 0;
	}

	public int CompareTo(DateTimeOffset other)
	{
		DateTime utcDateTime = other.UtcDateTime;
		DateTime utcDateTime2 = UtcDateTime;
		if (utcDateTime2 > utcDateTime)
		{
			return 1;
		}
		if (utcDateTime2 < utcDateTime)
		{
			return -1;
		}
		return 0;
	}

	public override bool Equals(object obj)
	{
		if (obj is DateTimeOffset)
		{
			return UtcDateTime.Equals(((DateTimeOffset)obj).UtcDateTime);
		}
		return false;
	}

	public bool Equals(DateTimeOffset other)
	{
		return UtcDateTime.Equals(other.UtcDateTime);
	}

	public bool EqualsExact(DateTimeOffset other)
	{
		if (ClockDateTime == other.ClockDateTime && Offset == other.Offset)
		{
			return ClockDateTime.Kind == other.ClockDateTime.Kind;
		}
		return false;
	}

	public static bool Equals(DateTimeOffset first, DateTimeOffset second)
	{
		return DateTime.Equals(first.UtcDateTime, second.UtcDateTime);
	}

	public static DateTimeOffset FromFileTime(long fileTime)
	{
		return new DateTimeOffset(DateTime.FromFileTime(fileTime));
	}

	public static DateTimeOffset FromUnixTimeSeconds(long seconds)
	{
		if (seconds < -62135596800L || seconds > 253402300799L)
		{
			throw new ArgumentOutOfRangeException("seconds", string.Format(Environment.GetResourceString("Valid values are between {0} and {1}, inclusive."), -62135596800L, 253402300799L));
		}
		return new DateTimeOffset(seconds * 10000000 + 621355968000000000L, TimeSpan.Zero);
	}

	public static DateTimeOffset FromUnixTimeMilliseconds(long milliseconds)
	{
		if (milliseconds < -62135596800000L || milliseconds > 253402300799999L)
		{
			throw new ArgumentOutOfRangeException("milliseconds", string.Format(Environment.GetResourceString("Valid values are between {0} and {1}, inclusive."), -62135596800000L, 253402300799999L));
		}
		return new DateTimeOffset(milliseconds * 10000 + 621355968000000000L, TimeSpan.Zero);
	}

	void IDeserializationCallback.OnDeserialization(object sender)
	{
		try
		{
			m_offsetMinutes = ValidateOffset(Offset);
			m_dateTime = ValidateDate(ClockDateTime, Offset);
		}
		catch (ArgumentException innerException)
		{
			throw new SerializationException(Environment.GetResourceString("An error occurred while deserializing the object.  The serialized data is corrupt."), innerException);
		}
	}

	[SecurityCritical]
	void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context)
	{
		if (info == null)
		{
			throw new ArgumentNullException("info");
		}
		info.AddValue("DateTime", m_dateTime);
		info.AddValue("OffsetMinutes", m_offsetMinutes);
	}

	private DateTimeOffset(SerializationInfo info, StreamingContext context)
	{
		if (info == null)
		{
			throw new ArgumentNullException("info");
		}
		m_dateTime = (DateTime)info.GetValue("DateTime", typeof(DateTime));
		m_offsetMinutes = (short)info.GetValue("OffsetMinutes", typeof(short));
	}

	public override int GetHashCode()
	{
		return UtcDateTime.GetHashCode();
	}

	public static DateTimeOffset Parse(string input)
	{
		TimeSpan offset;
		return new DateTimeOffset(DateTimeParse.Parse(input, DateTimeFormatInfo.CurrentInfo, DateTimeStyles.None, out offset).Ticks, offset);
	}

	public static DateTimeOffset Parse(string input, IFormatProvider formatProvider)
	{
		return Parse(input, formatProvider, DateTimeStyles.None);
	}

	public static DateTimeOffset Parse(string input, IFormatProvider formatProvider, DateTimeStyles styles)
	{
		styles = ValidateStyles(styles, "styles");
		TimeSpan offset;
		return new DateTimeOffset(DateTimeParse.Parse(input, DateTimeFormatInfo.GetInstance(formatProvider), styles, out offset).Ticks, offset);
	}

	public static DateTimeOffset ParseExact(string input, string format, IFormatProvider formatProvider)
	{
		return ParseExact(input, format, formatProvider, DateTimeStyles.None);
	}

	public static DateTimeOffset ParseExact(string input, string format, IFormatProvider formatProvider, DateTimeStyles styles)
	{
		styles = ValidateStyles(styles, "styles");
		TimeSpan offset;
		return new DateTimeOffset(DateTimeParse.ParseExact(input, format, DateTimeFormatInfo.GetInstance(formatProvider), styles, out offset).Ticks, offset);
	}

	public static DateTimeOffset ParseExact(string input, string[] formats, IFormatProvider formatProvider, DateTimeStyles styles)
	{
		styles = ValidateStyles(styles, "styles");
		TimeSpan offset;
		return new DateTimeOffset(DateTimeParse.ParseExactMultiple(input, formats, DateTimeFormatInfo.GetInstance(formatProvider), styles, out offset).Ticks, offset);
	}

	public TimeSpan Subtract(DateTimeOffset value)
	{
		return UtcDateTime.Subtract(value.UtcDateTime);
	}

	public DateTimeOffset Subtract(TimeSpan value)
	{
		return new DateTimeOffset(ClockDateTime.Subtract(value), Offset);
	}

	public long ToFileTime()
	{
		return UtcDateTime.ToFileTime();
	}

	public long ToUnixTimeSeconds()
	{
		return UtcDateTime.Ticks / 10000000 - 62135596800L;
	}

	public long ToUnixTimeMilliseconds()
	{
		return UtcDateTime.Ticks / 10000 - 62135596800000L;
	}

	public DateTimeOffset ToLocalTime()
	{
		return ToLocalTime(throwOnOverflow: false);
	}

	internal DateTimeOffset ToLocalTime(bool throwOnOverflow)
	{
		return new DateTimeOffset(UtcDateTime.ToLocalTime(throwOnOverflow));
	}

	public override string ToString()
	{
		return DateTimeFormat.Format(ClockDateTime, null, DateTimeFormatInfo.CurrentInfo, Offset);
	}

	public string ToString(string format)
	{
		return DateTimeFormat.Format(ClockDateTime, format, DateTimeFormatInfo.CurrentInfo, Offset);
	}

	public string ToString(IFormatProvider formatProvider)
	{
		return DateTimeFormat.Format(ClockDateTime, null, DateTimeFormatInfo.GetInstance(formatProvider), Offset);
	}

	public string ToString(string format, IFormatProvider formatProvider)
	{
		return DateTimeFormat.Format(ClockDateTime, format, DateTimeFormatInfo.GetInstance(formatProvider), Offset);
	}

	public DateTimeOffset ToUniversalTime()
	{
		return new DateTimeOffset(UtcDateTime);
	}

	public static bool TryParse(string input, out DateTimeOffset result)
	{
		DateTime result2;
		TimeSpan offset;
		bool result3 = DateTimeParse.TryParse(input, DateTimeFormatInfo.CurrentInfo, DateTimeStyles.None, out result2, out offset);
		result = new DateTimeOffset(result2.Ticks, offset);
		return result3;
	}

	public static bool TryParse(string input, IFormatProvider formatProvider, DateTimeStyles styles, out DateTimeOffset result)
	{
		styles = ValidateStyles(styles, "styles");
		DateTime result2;
		TimeSpan offset;
		bool result3 = DateTimeParse.TryParse(input, DateTimeFormatInfo.GetInstance(formatProvider), styles, out result2, out offset);
		result = new DateTimeOffset(result2.Ticks, offset);
		return result3;
	}

	public static bool TryParseExact(string input, string format, IFormatProvider formatProvider, DateTimeStyles styles, out DateTimeOffset result)
	{
		styles = ValidateStyles(styles, "styles");
		DateTime result2;
		TimeSpan offset;
		bool result3 = DateTimeParse.TryParseExact(input, format, DateTimeFormatInfo.GetInstance(formatProvider), styles, out result2, out offset);
		result = new DateTimeOffset(result2.Ticks, offset);
		return result3;
	}

	public static bool TryParseExact(string input, string[] formats, IFormatProvider formatProvider, DateTimeStyles styles, out DateTimeOffset result)
	{
		styles = ValidateStyles(styles, "styles");
		DateTime result2;
		TimeSpan offset;
		bool result3 = DateTimeParse.TryParseExactMultiple(input, formats, DateTimeFormatInfo.GetInstance(formatProvider), styles, out result2, out offset);
		result = new DateTimeOffset(result2.Ticks, offset);
		return result3;
	}

	private static short ValidateOffset(TimeSpan offset)
	{
		long ticks = offset.Ticks;
		if (ticks % 600000000 != 0L)
		{
			throw new ArgumentException(Environment.GetResourceString("Offset must be specified in whole minutes."), "offset");
		}
		if (ticks < -504000000000L || ticks > 504000000000L)
		{
			throw new ArgumentOutOfRangeException("offset", Environment.GetResourceString("Offset must be within plus or minus 14 hours."));
		}
		return (short)(offset.Ticks / 600000000);
	}

	private static DateTime ValidateDate(DateTime dateTime, TimeSpan offset)
	{
		long num = dateTime.Ticks - offset.Ticks;
		if (num < 0 || num > 3155378975999999999L)
		{
			throw new ArgumentOutOfRangeException("offset", Environment.GetResourceString("The UTC time represented when the offset is applied must be between year 0 and 10,000."));
		}
		return new DateTime(num, DateTimeKind.Unspecified);
	}

	private static DateTimeStyles ValidateStyles(DateTimeStyles style, string parameterName)
	{
		if (((uint)style & 0xFFFFFF00u) != 0)
		{
			throw new ArgumentException(Environment.GetResourceString("An undefined DateTimeStyles value is being used."), parameterName);
		}
		if ((style & DateTimeStyles.AssumeLocal) != 0 && (style & DateTimeStyles.AssumeUniversal) != 0)
		{
			throw new ArgumentException(Environment.GetResourceString("The DateTimeStyles values AssumeLocal and AssumeUniversal cannot be used together."), parameterName);
		}
		if ((style & DateTimeStyles.NoCurrentDateDefault) != 0)
		{
			throw new ArgumentException(Environment.GetResourceString("The DateTimeStyles value 'NoCurrentDateDefault' is not allowed when parsing DateTimeOffset."), parameterName);
		}
		style &= ~DateTimeStyles.RoundtripKind;
		style &= ~DateTimeStyles.AssumeLocal;
		return style;
	}

	public static implicit operator DateTimeOffset(DateTime dateTime)
	{
		return new DateTimeOffset(dateTime);
	}

	public static DateTimeOffset operator +(DateTimeOffset dateTimeOffset, TimeSpan timeSpan)
	{
		return new DateTimeOffset(dateTimeOffset.ClockDateTime + timeSpan, dateTimeOffset.Offset);
	}

	public static DateTimeOffset operator -(DateTimeOffset dateTimeOffset, TimeSpan timeSpan)
	{
		return new DateTimeOffset(dateTimeOffset.ClockDateTime - timeSpan, dateTimeOffset.Offset);
	}

	public static TimeSpan operator -(DateTimeOffset left, DateTimeOffset right)
	{
		return left.UtcDateTime - right.UtcDateTime;
	}

	public static bool operator ==(DateTimeOffset left, DateTimeOffset right)
	{
		return left.UtcDateTime == right.UtcDateTime;
	}

	public static bool operator !=(DateTimeOffset left, DateTimeOffset right)
	{
		return left.UtcDateTime != right.UtcDateTime;
	}

	public static bool operator <(DateTimeOffset left, DateTimeOffset right)
	{
		return left.UtcDateTime < right.UtcDateTime;
	}

	public static bool operator <=(DateTimeOffset left, DateTimeOffset right)
	{
		return left.UtcDateTime <= right.UtcDateTime;
	}

	public static bool operator >(DateTimeOffset left, DateTimeOffset right)
	{
		return left.UtcDateTime > right.UtcDateTime;
	}

	public static bool operator >=(DateTimeOffset left, DateTimeOffset right)
	{
		return left.UtcDateTime >= right.UtcDateTime;
	}
}
